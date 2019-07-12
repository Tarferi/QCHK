# Snipers Power Tool
[The official thread](http://www.staredit.net/topic/17849/)

This contains archive for the latest development version of the project.<br />
This project contains also sources for the DLL file, because there are no crackers inside
This source contains unreleased features, with some bugs created, removed, or worse, undetermined.

#### Known limitations of SPT:

* STR if f\*\*ked up and you can only have like 5kb of your own string data (should be enough for no very talkative maps)
* LOC is f\*\*ked up (you can only use like 200 locations instead of full 255, relocations happen)
* UPRP is f\*\*ked up (this is a work on progress). Using triggers like "Create units with properties" is somehow untested feature (properties are copied and relocated, but further untested) 
* You will probably need to use additional compressor to deflate the final map file (work in progress as well)
* Time lock triggers crash after map expires? (perhaps a feature)
* If you used EUD Editor to alter any aspect of the map, you should expect undefined behavior (by this tool and even sc)
* EPD Editor is fine though. EMP is parasite, so you can even mod it further with it
* Slot 7 must be a computer slot (has the EUD stuff). Slot 8 doesn't get abilities (work in progress, migrating triggers from slot 7 to 8 produces errors)
* Condition "Elapsed time is at least 3 seconds" will be added to all your triggers, so if there is a trigger with 16 conditions, expect a crash
* Barrier always restores health to 12 HP (see [report of memory analysis of STR section EUD part](https://pastebin.com/XAGtMzrD)) and this is unlikely to ever change 
* Touch revive not working for some reason, locations get damaged for some reason, map crashes sometimes for some reason
