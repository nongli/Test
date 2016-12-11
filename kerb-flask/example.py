#!/usr/bin/env python

import sys
from flask import Flask
from cerebro_flask_kerberos import init_kerberos
from cerebro_flask_kerberos import requires_authentication

DEBUG=True

app = Flask(__name__)
app.config.from_object(__name__)


@app.route("/")
@requires_authentication
def index(user):
  return "Hello " + user

@app.route("/unsafe")
def unsafe():
  return "Unsafe"


if __name__ == '__main__':
  if not init_kerberos(app):
    print("Could not initialize kerberos.")
    sys.exit(1)
  app.run(host='0.0.0.0')
