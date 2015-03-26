import zmq
import random
import uuid


ctx = zmq.Context()
socket = ctx.socket(zmq.DEALER)
socket.identity = uuid.uuid4().hex.encode()[0:8]
addr = "tcp://localhost:6666"
#addr = "ipc:///tmp/sl_requester.socket"
print("connecting to " + addr)
socket.connect(addr)
print("connected.")

i= 3
jobID = b'\x00\x00\x00\x00' #this is an int so must be 32bit
subject = b'\x04\x00\x00\x00' #as above
batchID = chr(i).encode() # is just a string so can be whatevs
globalData = b"harro"
localData = b"prease"
msg = [batchID, b"", subject, jobID, globalData, localData]
print("sending message...", msg)
socket.send_multipart(msg)
print("message sent. recieving...")
r = socket.recv_multipart()
print("message recieved", r)
