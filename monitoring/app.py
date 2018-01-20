import socket
import select
import Queue
from flask import Flask
from celery import Celery
import json


def make_celery(app):
    celery = Celery(
        app.import_name,
        backend='redis://localhost',
        broker='pyamqp://'
    )
    celery.conf.update(app.config)
    TaskBase = celery.Task
    class ContextTask(TaskBase):
        abstract = True
        def __call__(self, *args, **kwargs):
            with app.app_context():
                return TaskBase.__call__(self, *args, **kwargs)
    celery.Task = ContextTask
    return celery



app = Flask(__name__)
socket_queue = Queue.Queue()
celery_app = make_celery(app)


@celery_app.task()
def listen_to_udp():
    """
    This code was taken from
    https://stackoverflow.com/questions/9969259/python-raw-socket-listening-for-udp-packets-only-half-of-the-packets-received
    """
    s1 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s1.bind(('0.0.0.0', 8080))
    s2 = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_UDP)
    s2.bind(('0.0.0.0', 8080))
    while True:
        r, w, x = select.select([s1, s2], [], [])
        for i in r:
            socket_queue.put((i, i.recvfrom(131072)))



@celery_app.task
def record_udp_message(machine_id, pool, address, success):
	redis = redis.Client()
	redis.hmset("login", machine_id, json.dumps(dict(pool=pool, address=address, success=success)))
	if success:
		redis.incr("login_success")
	else:
		redis.incr("login_failure")




@app.route("/")
def test_home():
    listen_to_udp.delay()
    print(socket_queue.get())

if __name__ == "__main__":
    #run install.py to install dependencies and create the database
    app.run(host="0.0.0.0", port=5000, debug=True)
