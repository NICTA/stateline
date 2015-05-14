import zmq
import random
import uuid

HELLO = b'0'
HEARTBEAT = b'1'
REQUEST = b'2'
JOB = b'3'
RESULT = b'4'
GOODBYE = b'5'


ctx = zmq.Context()
socket = ctx.socket(zmq.DEALER)
socket.identity = uuid.uuid4().hex.encode()[0:8]
# addr = "tcp://localhost:6666"
addr = "ipc:///tmp/sl_delegator.socket"
print("connecting to " + addr)
socket.connect(addr)
print("connected.")


while True:
    # jobTypes = b'gravity:mag:mt'
    jobTypes = b'job'
    subject = REQUEST
    batchID = str(0).encode()
    data = str(random.random()).encode()
    msg = [batchID, b"", REQUEST, jobTypes, data]
    print("sending ", msg)
    socket.send_multipart(msg)
    r = socket.recv_multipart()
    print("received ", r)
