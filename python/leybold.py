import time
import PyTkModLabLeybold

leybold = PyTkModLabLeybold.LeyboldGraphixThree("/dev/ttyLeybold")

print "version:   " + leybold.GetVersion()

print "date:      " + leybold.GetDate()
print "time:      " + leybold.GetTime()

