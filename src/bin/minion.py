import zmq
import random
import uuid


ctx = zmq.Context()
socket = ctx.socket(zmq.DEALER)
socket.identity = uuid.uuid4().hex.encode()[0:8]
#addr = "ipc:///tmp/sl_minion.socket"
addr = "tcp://localhost:7777"
print("connecting to ", addr)
socket.connect(addr)
print("connected.")
        
jobID = b'\x00\x00\x00\x00'
subject = b'\x03\x00\x00\x00' # JOBREQUEST
msg = [b"", subject, jobID]
print("sending message", msg)
socket.send_multipart(msg)
print("sent. Receiving...")
r = socket.recv_multipart()
print("Message received", r)
