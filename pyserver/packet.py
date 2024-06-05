import struct
from config import *

def pack_refresh(server, refresh_idx):
    pkt_count = server.packet_count
    num_pkts = pkt_count - refresh_idx

    buf = [REFRESH, pkt_count]

    if (refresh_idx == -1
        or num_pkts >= PACKET_HISTORY_SIZE
        or num_pkts <= 0):
        packstr = "ii"
    elif num_pkts < PACKET_HISTORY_SIZE:
        first_idx = refresh_idx % PACKET_HISTORY_SIZE
        last_idx = pkt_count % PACKET_HISTORY_SIZE
        if first_idx < last_idx:
            buf.append(b''.join(server.packets[first_idx:last_idx]))
        else:
            buf.append(b''.join(server.packets[first_idx:]))
            buf[-1] += b''.join(server.packets[:last_idx])
        packstr = "ii{}s".format(num_pkts*PACKET_SIZE)

    return struct.pack(packstr, *buf)

# STATISTICS UBIT Name Most Active Most Active Count Refresh Count Invalid Count Message Count
def pack_statistics(server):
    # TODO: The first two fields are the header, this is a relic of the ugly C code
    # and should probably be designed out if the client is ever refactored
    statfmt = "iii{NS}s{NS}siqqi".format(NS=NAME_SIZE)
    return struct.pack(
        statfmt,
        STATISTICS,
        1, # There is only one packet to unpack
        STATISTICS,
        SERVER_UNAME.encode(),
        server.most_active.encode(),
        server.most_active_count,
        server.invalid_count,
        server.refresh_count,
        server.packet_count
    )

def pack_message(msg):
    # TODO: The first two fields are the header, this is a relic of the ugly C code
    # and should probably be designed out if the client is ever refactored
    msg_len = len(msg)
    statfmt = "=iii{NS}sqq{msg_len}s".format(NS=NAME_SIZE, msg_len=msg_len)
    return struct.pack(
        statfmt,
        MESSAGE,
        1, # There is only one packet to unpack
        MESSAGE,
        SERVER_UNAME.encode(),
        msg_len,
        0,
        msg.encode()
    )

# PacketTypes contain the following data in a dictionary:
# fields (int): Number of data lengths and data to read
# format (string): Format string to be used to generate text output and logs.
#   Format strings will always have the first spot reserved for the UBIT, then
#   `fields` spots for the unpacked packed fields
PacketTypes = {}
PacketTypes[REFRESH] = {"fields": -1, "format": None}
PacketTypes[STATUS] = {"fields": 1, "format": "{} {}"}
PacketTypes[MESSAGE] = {"fields": 1, "format": "{}: {}"}
PacketTypes[LABELED] = {"fields": 2, "format": "{}: @{} {}"}
PacketTypes[STATISTICS] = {"fields": -1, "format": None}

def unpack(packet):
    """Unpack the given packet

    Parameters:
    packet (bytes): The bytes object recieved by the socket

    Returns: Tuple of form (valid, unpacked, refresh_idx)
    valid (boolean): True if the packet is valid, False otherwise
    unpacked (string): unpacked string to be used for debugging/logging purposes
    refresh_idx(int): indicates the last index the sender recieved, only
                      present in refresh packets
    """

    FAILURE = (False, None, None)

    headfmt = "i{}s".format(NAME_SIZE)
    headsize = struct.calcsize(headfmt)
    ptype, ubit = struct.unpack(headfmt, packet[:headsize])

    if ptype not in PacketTypes:
        print("Invalid packet type {} recieved".format(ptype))
        return FAILURE

    if ptype == STATISTICS:
        return (True, None, None)

    if ptype == REFRESH:
        idxfmt = "i"
        refresh_idx = struct.unpack_from(idxfmt, packet, headsize)
        return (True, None, refresh_idx[0])

    # Decode data lengths based on packet type
    lengthsfmt = "{}NN".format(PacketTypes[ptype]["fields"])
    lengthssize = struct.calcsize(lengthsfmt)
    lengths = struct.unpack_from(lengthsfmt, packet, headsize)

    if lengths[-1] != 0:
        print("No zero terminator found on data length list")
        return FAILURE

    totalLen = sum(lengths)
    if totalLen > MAX_MESSAGE_SIZE:
        print("Summed data lengths ({}) would overflow packet size".format(totalLen))
        return FAILURE

    bodyfmt = ""
    for sz in lengths[:-1]:
        if sz == 0:
            print("Data length list included an unexpected zero value")
            return FAILURE
        bodyfmt += "{}s".format(sz)

    body = struct.unpack_from(bodyfmt, packet, headsize+lengthssize)

    for st in body:
        for ch in st:
            if ch < 0x20 or ch > 0x7e or ch == b'\t':
                # 255 is sent when the user exits, don't print when it
                # is encountered
                if ch != 255:
                    print("Invalid character found: 0x{:02x}".format(ch))
                return FAILURE

    try:
        stringified = PacketTypes[ptype]["format"].format(
            ubit.decode().rstrip('\0'), *[b.decode() for b in body[::-1]]
        )
    except:
        print("Failed to decode text")
        return FAILURE

    return (True, stringified, None)

# Check if the ubit matches the sender's username
#
# Returns: True if valid, False otherwise
def validate_ubit(packet, ubit):
    """Check if the ubit in the packet matches the username of the sender

    Parameters:
    packet (bytes): The bytes object recieved by the socket
    ubit (string): The name of the user who sent packet

    Returns: True if the ubit in packet matches 'ubit'
    """

    # Convert given ubit to bytes array of fixed size
    headfmt = "{}s".format(NAME_SIZE)
    headsize = struct.calcsize(headfmt)
    packed_ubit = struct.pack(headfmt, ubit.encode())

    # Compare to value in packet
    return packed_ubit == packet[4:4+NAME_SIZE]
