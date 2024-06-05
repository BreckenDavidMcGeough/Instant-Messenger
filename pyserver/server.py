#!/usr/bin/env python3
import os
import sys
import packet
import struct
import logging
import threading

from config import *
from io import BytesIO
from pwd import getpwuid
from signal import signal, SIGINT
from ctypes import Structure, c_uint32, sizeof
from time import clock_gettime, CLOCK_MONOTONIC_RAW
from socket import SOL_SOCKET, SO_PASSCRED, SO_PEERCRED
from socketserver import UnixDatagramServer, BaseRequestHandler

SCM_CREDENTIALS = 2 # Python does not export this value

# Set up logging
logging.basicConfig(filename="Messages.log", level=logging.INFO)

# The ucred structure is not standardized across *nix'es. i.e in linux
# it is {pid, uid, gid}, but in openbsd it is {uid, gid, pid}.  In
# other BSDs it may contain an array of gids instead of a single
# value.  If this program is not run on linux, a new ucred struct
# should be created and used in the call to recvmsg in get_request().
# NOTE: I use ctypes structures here to clearly delineate that this is
# a well defined system structure as opposed to arbitrarily defined
# structures used for the assignment. This can just as easily bnne a
# struct in a namedtuple.
class ucred_linux_t(Structure):
    _fields_ = [("pid", c_uint32),
                ("uid", c_uint32),
                ("gid", c_uint32)
    ]

class User():

    def __init__(self, name):
        self.name = name
        self.packets = 0
        self.recent_packets = [0] * RATE_LIMIT
        self.limit = 0


    def add_packet(self, packet):
        cur_time = clock_gettime(CLOCK_MONOTONIC_RAW)
        if self.limit > cur_time:
            return False

        self.packets += 1

        idx = self.packets % RATE_LIMIT
        oldest = (self.packets+1) % RATE_LIMIT
        self.recent_packets[idx] = cur_time

        if self.recent_packets[idx] - self.recent_packets[oldest] <= 1:
            self.limit = cur_time + LOCKOUT
            return False

        return True

class PacketHandler(BaseRequestHandler):

    # Stolen from cpython's DatagramRequestHandler implementation.
    def setup(self):
        # Username is the value of the CSE220_UBIT environment variable,
        # or the user's username.
        self.uname = os.getenv("CSE220_UBIT",
                               getpwuid(self.server.uc.uid).pw_name)
        self.packet, self.socket = self.request
        self.rfile = BytesIO(self.packet)
        self.wfile = BytesIO()
        self.respond = False

    # Handle Request
    # Read data from BytesIO(BufferedIOBase) self.rfile
    # Write data to BytesIO(BufferedIOBase) self.wfile
    def handle(self):
        valid_ubit = packet.validate_ubit(self.packet, self.uname)
        if valid_ubit == False:
            print("Invalid username recieved, ensure\
your username field contains your UBIT name")
            self.server.invalid_count += 1
            return

        # Register user
        if self.uname not in self.server.users:
            self.server.users[self.uname] = User(self.uname)

        user = self.server.users[self.uname]

        # Unpack and validate packet
        valid, unpacked, refresh_idx = packet.unpack(self.packet)
        if valid == False:
            self.server.invalid_count += 1
            return

        # If this is a packet which is sent to other users, i.e it
        # can be stringified
        if unpacked is not None:
            logging.info(unpacked)
            
            # Rate Limit if the packet is of a type that is sent to others
            allowed = user.add_packet(self.packet)

            # If user is locked out do not continue
            if not allowed:
                print("User {} is locked out".format(self.uname))

                self.respond = True
                resp = packet.pack_message(
                    "You are locked out for {} more seconds"
                    .format((int)(user.limit-clock_gettime(CLOCK_MONOTONIC_RAW))))
                self.wfile.write(resp)
                return

            if user.packets > self.server.most_active_count:
                self.server.most_active_count = user.packets
                self.server.most_active = user.name

            pkt_idx = self.server.packet_count % PACKET_HISTORY_SIZE
            self.server.packets[pkt_idx] = self.packet
            self.server.packet_count += 1
        else:
            if refresh_idx is None:
                resp = packet.pack_statistics(self.server)
            else:
                self.server.refresh_count += 1
                resp = packet.pack_refresh(self.server, refresh_idx)

            self.wfile.write(resp)
            self.respond = True

    # Respond to client with contents of self.wfile
    def finish(self):
        if self.client_address is not None and self.respond == True:
            self.socket.sendto(self.wfile.getvalue(),
                               self.client_address)

# Packet server based on the UnixDatagramServer
# Overwrite some methods to get access to ancillary data used for
# username validation
class IMServer(UnixDatagramServer):

    # This is called on server startup, it is empty for
    # UnixDatagramServer. Used in this case to allow sending
    # credentials.
    def server_activate(self):
        self.socket.setsockopt(SOL_SOCKET, SO_PASSCRED, 1)
        self.invalid_count = 0
        self.refresh_count = 0
        self.users = {}
        self.most_active = None
        self.most_active_count = 0
        self.packet_count = 0
        self.packets = [None] * PACKET_HISTORY_SIZE

    # Overwrite get_request to use recvmsg and read ancillary data to
    # get SCM_CREDENTIALS for user validation.
    def get_request(self):
        data, ancillary, flags, client_addr = self.socket.recvmsg(
            self.max_packet_size,
            32 # size of ancillary data buffer
        )

        # TODO: Check flags?

        for cmsg_level, cmsg_type, cmsg_data in ancillary:
            if cmsg_level == SOL_SOCKET and cmsg_type == SCM_CREDENTIALS:
                # Sets self.uc in PacketHandler instance.
                self.uc = ucred_linux_t.from_buffer_copy(cmsg_data)

        return (data, self.socket), client_addr

def main(argv):
    if len(argv) != 2:
        print("Usage: {} <filename>".format(argv[0]))
        sys.exit(1)

    if not sys.platform.startswith("linux"):
            print("Not running on linux system,\
the ucred structure will need to be updated")
            sys.exit(1)

    filename = os.path.abspath(argv[1])
    print("Starting server on {}".format(filename))

    # See python socketserver documentation
    with IMServer(filename, PacketHandler) as server:
        def handle_signal(sig, frame):
            print("shutting down")
            server.shutdown()

        signal(SIGINT, handle_signal)
        os.chmod(filename, 0o666);
        try:
            # Start server in new thread
            server_thread = threading.Thread(target=server.serve_forever)
            server_thread.start()
        except Exception as e:
            print("Error: {}".format(str(e)))
            server.shutdown()

        server_thread.join()

    if os.path.exists(filename):
        print("Removing {}".format(filename))
        os.remove(filename)

if __name__ == "__main__":
    main(sys.argv)
