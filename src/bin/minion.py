import zmq
import random
import uuid

HELLO = b'\x00\x00\x00\x00b'
HEARTBEAT = b'\x01\x00\x00\x00b'
PROBLEMSPEC = b'\x02\x00\x00\x00b'
JOBREQUEST = b'\x03\x00\x00\x00b'
JOB = b'\x04\x00\x00\x00b'
JOBSWAP = b'\x05\x00\x00\x00b'
ALLDONE = b'\x06\x00\x00\x00b'
GOODBYE = b'\x07\x00\x00\x00b'

ctx = zmq.Context()
socket = ctx.socket(zmq.DEALER)
socket.identity = uuid.uuid4().hex.encode()[0:8]
#addr = "ipc:///tmp/sl_minion.socket"
addr = "tcp://localhost:7777"
print("connecting to ", addr)
socket.connect(addr)
print("connected.")
        
jobID = b'\x00\x00\x00\x00'
subject = JOBREQUEST
msg = [b"", subject, jobID]
print("sending message", msg)
socket.send_multipart(msg)
print("sent. Receiving...")
r = socket.recv_multipart()
print("Message received", r)

address = r[0:2]
#do some work
print("doing work...")
rmsg = address + [b"", JOBSWAP, b"result_data", jobID]
print("sending result...")
socket.send_multipart(rmsg)

import IPython; IPython.embed(); import sys; sys.exit()
