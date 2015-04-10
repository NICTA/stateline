import zmq
import random
import uuid
import time

HELLO = b'0'
HEARTBEAT = b'1'
REQUEST = b'2'
JOB = b'3'
RESULT = b'4'
GOODBYE = b'5'


ctx = zmq.Context()
socket = ctx.socket(zmq.DEALER)
socket.identity = uuid.uuid4().hex.encode()[0:8]
addr = "ipc:///tmp/sl_worker.socket"
# addr = "tcp://localhost:7777"
print("connecting to ", addr)
socket.connect(addr)
print("connected.")
        
jobTypes = b'gravity:mag:mt'
socket.send_multipart([b"", HELLO, jobTypes])

r = socket.recv_multipart()
print("Message received", r)

msg = [b"", subject, jobTypes]
print("sending message", msg)
socket.send_multipart(msg)
print("sent. Receiving...")

address = r[0:2]
#do some work
print("doing work...")
time.sleep(10)

rmsg = address + [b"", WORK, r[3], b"result_data"]
print("sending result...")
socket.send_multipart(rmsg)

import IPython; IPython.embed(); import sys; sys.exit()

