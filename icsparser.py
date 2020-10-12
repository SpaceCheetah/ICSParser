import datetime
from datetime import timedelta
from dateutil.parser import parse
from dateutil.rrule import rrulestr
import requests

class Event:
    summary = ""
    startTime = datetime.datetime.now()
    endTime = datetime.datetime.now()
    def isValid(self):
        return self.summary != "" and self.startTime != 0 and self.endTime > datetime.datetime.today() and self.startTime < self.endTime and self.startTime < (datetime.datetime.today() + timedelta(days=7))

def parseDate(line):
    return parse(line[(line.rfind(":") + 1):])

events = []
rrule = ""
inEvent = False
current = Event()
lines = requests.get("https://outlook.live.com/owa/calendar/00000000-0000-0000-0000-000000000000/f102d07d-d896-40f2-bdd0-984be07642bc/cid-C7854B2BABE873DB/calendar.ics").text.split("\r\n")
for line in lines:
    if inEvent:
        if line == "END:VEVENT":
            inEvent = False
            if rrule == "":
                if current.isValid():
                    events.append(current)
            else:
                difference = current.endTime - current.startTime
                for time in list(rrulestr(rrule,dtstart=current.startTime,cache=True,ignoretz=True)):
                    c = Event()
                    c.startTime = time
                    c.endTime = time + difference
                    c.summary = current.summary
                    if c.isValid():
                        events.append(c)
                rrule = ""
            current = Event()
        elif line.startswith("SUMMARY:"):
            current.summary = line[8:]
        elif line.startswith("DTSTART;"):
            current.startTime = parseDate(line)
        elif line.startswith("DTEND;"):
            current.endTime = parseDate(line)
        elif line.startswith("RRULE:"):
            rrule = line[6:]
    elif line == "BEGIN:VEVENT":
        inEvent = True
for event in events:
    print("Event:\n\tSummary: ",end="")
    print(event.summary,end="")
    print("\n\tStart Time: ",end="")
    print(event.startTime,end="")
    print("\n\tEnd Time: ",end="")
    print(event.endTime)