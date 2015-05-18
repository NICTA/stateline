import zmq
import json
import logging
import numpy as np
import subprocess

HELLO = b'0'
HEARTBEAT = b'1'
REQUEST = b'2'
JOB = b'3'
RESULT = b'4'
GOODBYE = b'5'

def nll(x):
    return x.dot(x)

def handle_job(job_type, job_data):
    sample = list(map(float, job_data.split(b':')))
    return nll(np.asarray(sample))

def send_hello(socket, jobTypes):
    jobTypesStr = ':'.join(jobTypes).encode('ascii')

    logging.info("Sending HELLO message...")
    socket.send_multipart([b"", HELLO, jobTypesStr])

def job_loop(socket):
    while True:
        logging.info("Getting job...")
        r = socket.recv_multipart()
        logging.info("Got job!")

        assert len(r) == 5

        subject, job_type, job_id, job_data = r[1:]

        assert subject == JOB

        result = handle_job(job_type, job_data)

        logging.info("Sending result...")
        rmsg = [b"", RESULT, job_id, str(result).encode('ascii')]
        socket.send_multipart(rmsg)
        logging.info("Sent result {0}!".format(job_id))

def main():
    # Initiate logging
    #logging.basicConfig(level=logging.DEBUG)
    logging.basicConfig(level=logging.CRITICAL)

    # Launch stateline-client
    logging.info('Starting client')
    client_proc = subprocess.Popen(['./stateline-client'])
    logging.info('Started client')

    with open('config.json', 'r') as f:
        config = json.load(f)

    jobTypes = config['jobTypes']

    ctx = zmq.Context()
    socket = ctx.socket(zmq.DEALER)
    addr = "ipc:///tmp/sl_worker.socket"

    logging.info("Connecting to {0}...".format(addr))
    socket.connect(addr)
    logging.info("Connected!")

    send_hello(socket, jobTypes)
    job_loop(socket)

if __name__ == "__main__":
    main()
