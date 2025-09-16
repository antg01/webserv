#NEEDS TO BE ADAPTED


#!/usr/bin/env python3
import os, sys
print("Status: 200 OK")
print("Content-Type: text/plain")
print()
print("Method:", os.environ.get("REQUEST_METHOD",""))
print("Path:", os.environ.get("PATH_INFO",""))
print("Query:", os.environ.get("QUERY_STRING",""))
body = sys.stdin.read()
print("Body:", body)
