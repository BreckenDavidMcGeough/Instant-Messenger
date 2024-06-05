# You probably don't want to modify any of this...

# Maximum messages per second. Refresh packets are not counted.
RATE_LIMIT = 5

# Number of seconds to lock user out for if they go over RATE_LIMIT
LOCKOUT = 300

# Size of the name field
NAME_SIZE = 16

# Packet type definitions
# Do not change these (unless you are changing the packet types)
REFRESH = 0
STATUS = 1
MESSAGE = 2
LABELED = 3
STATISTICS = 4

# Maximum message size in bytes. This is the sum of the data lengths,
# _not_ the size of the entire packet.
MAX_MESSAGE_SIZE = 256

# Size of the packet in bytes. This is a static value and _must_ be
# the same as the value on the server
PACKET_SIZE = 1024

# Number of previous messages to store
PACKET_HISTORY_SIZE = 256

# Username used for messages from the server
# This should not be more than 16 characters long
SERVER_UNAME = "CSE220_ServerBot"
