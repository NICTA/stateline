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
# socket.identity = uuid.uuid4().hex.encode()[0:8]
addr = "ipc:///tmp/sl_worker.socket"
# addr = "tcp://localhost:7777"
print("connecting to ", addr)
socket.connect(addr)
print("connected.")
        
jobTypes = b'gravity:mag:mt'
socket.send_multipart([b"", HELLO, jobTypes])


while True:
    print("trying to get some work")
    r = socket.recv_multipart()
    print("Job received", r)
    time.sleep(3)
    rmsg = [b"", RESULT, r[3], b"result_data"]
    print("sending ", rmsg)
    socket.send_multipart(rmsg)


