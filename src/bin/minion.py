import zmq
import random
import uuid

HELLO = b'\x00\x00\x00\x00b'
HEARTBEAT = b'\x01\x00\x00\x00b'
WORK = b'\x02\x00\x00\x00b'
GOODBYE = b'\x03\x00\x00\x00b'

ctx = zmq.Context()
socket = ctx.socket(zmq.DEALER)
socket.identity = uuid.uuid4().hex.encode()[0:8]
addr = "ipc:///tmp/sl_worker.socket"
# addr = "tcp://localhost:7777"
print("connecting to ", addr)
socket.connect(addr)
print("connected.")
        
jobType = b'gravity'
subject = WORK
msg = [b"", subject, jobType]
print("sending message", msg)
socket.send_multipart(msg)
print("sent. Receiving...")
r = socket.recv_multipart()
print("Message received", r)

address = r[0:2]
#do some work
print("doing work...")
rmsg = address + [b"", WORK, b"result_data", jobType]
print("sending result...")
socket.send_multipart(rmsg)

import IPython; IPython.embed(); import sys; sys.exit()
