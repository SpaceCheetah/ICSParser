from dateutil.rrule import *
from dateutil.parser import *
from datetime import *
import sys
import requests
ics = requests.get("https://outlook.live.com/owa/calendar/00000000-0000-0000-0000-000000000000/f102d07d-d896-40f2-bdd0-984be07642bc/cid-C7854B2BABE873DB/calendar.ics")
lines = ics.text.split("\r\n")
print(lines)
#for time in list(rrulestr(sys.argv[1],dtstart=date.fromtimestamp(int(sys.argv[2])),ignoretz=True)):
#    print(time.timestamp());