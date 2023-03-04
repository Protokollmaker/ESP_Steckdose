import urllib.request
from websocket import create_connection
import time

ip = input("What is the ip?")
if ip == "\n":
    ip ="192.168.178.123"
numHTTPrequests = input("How Many webside Requests?")
numWSrequests = input("WebsocketRequests?")

ws = create_connection(f"ws://192.168.178.123/ws")

print("sendingRequests")
for i in range(0, int(numWSrequests)):
    ws.send('{"eventtype":"setTimer","time":100,"Relay":0,"turn":false,"run":true}')
    result =  ws.recv()
    time.sleep(1000)
    print(".")
print("\n")

for i in range(0, int(numHTTPrequests)):
    contents = urllib.request.urlopen(f"http://192.168.178.123/").read()
    print(f"{contents}")
    time.sleep(300/1000)