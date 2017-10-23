from flask_sqlalchemy import SQLAlchemy
from sqlalchemy import ForeignKey
from sqlalchemy.orm import relationship, deferred

from hashlib import sha256

import json

db = SQLAlchemy()

class User(db.Model):
    __tablename__ = 'user'
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(128), unique=True)
    password = db.Column(db.String(128))

    insts = relationship("Instance", back_populates="user")

    def __init__(self, name, password):
        self.name = name
        self.password = sha256(password).hexdigest()

class Instance(db.Model):
    __tablename__ = 'instance'
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(128))
    ip = db.Column(db.String(128))
    key = db.Column(db.String(128))
    user_id = db.Column(ForeignKey('user.id'))
    _events = db.Column(db.String(0x1000), default='[]')

    user = relationship("User", back_populates="insts")

    def __init__(self, name, ip, key, user):
        self.name = name
        self.ip = ip
        self.key = key
        self.user_id = user.id

    @property
    def events(self):
        return json.loads(self._events)

    @events.setter
    def events(self, v):
        self._events = json.dumps(v)



