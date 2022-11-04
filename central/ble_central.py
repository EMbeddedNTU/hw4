# ble_scan_connect.py:
from bluepy.btle import Peripheral, UUID
from bluepy.btle import Scanner, DefaultDelegate
import time
import threading
import matplotlib.pyplot as plt
import matplotlib.animation as animation


HEART_RATE_INDEX = 0
BUTTON_STATE_INDEX = 1
MAG_STATE_X_INDEX = 2
MAG_STATE_Y_INDEX = 3
MAG_STATE_Z_INDEX = 4
HEART_RATE_BODY_LOCATION_INDEX = 5

BODY_LOCATION = ["OTHER", "CHEST", "WRIST", "FINGER", "HAND", "EAR_LOBE", "FOOT"]

chUUIDs = [0x2a37, 0xa001, 0xa003, 0xa004, 0xa005, 0x2a38]
chHandles = [-1, -1, -1, -1, -1, -1]

data = [-1, -1, -1, -1, -1, -1]
flagDataChange = True 

HEARTRATE_SERVICE_UUID = 0x180D
BUTTON_SERVICE_UUID = 0xA000
MAG_SERVICE_UUID = 0xA002
serviceList = [HEARTRATE_SERVICE_UUID, BUTTON_SERVICE_UUID, MAG_SERVICE_UUID]

class ScanDelegate(DefaultDelegate):
    def __init__(self):
        DefaultDelegate.__init__(self)
    def handleDiscovery(self, dev, isNewDev, isNewData):
        if isNewDev:
            print ("Discovered device", dev.addr)
        elif isNewData:
            print ("Received new data from", dev.addr)
    def handleNotification(self, cHandle, _data):
        global flagDataChange, data
        for idx in [HEART_RATE_INDEX, BUTTON_STATE_INDEX]:
            newData = int.from_bytes(_data, "big")
            if cHandle == chHandles[idx] and data[idx] != newData: 
               flagDataChange = True
               data[idx] = newData
        for idx in [MAG_STATE_X_INDEX, MAG_STATE_Y_INDEX, MAG_STATE_Z_INDEX]:
            newData = int.from_bytes(_data, "little", signed=True)
            if cHandle == chHandles[idx] and data[idx] != newData: 
               flagDataChange = True
               data[idx] = newData
        if flagDataChange:
            if data[HEART_RATE_INDEX] != -1:
                print("heart rate data", data[HEART_RATE_INDEX])
            if data[BUTTON_STATE_INDEX] != -1:
                print("button data", data[BUTTON_STATE_INDEX])
            if data[MAG_STATE_X_INDEX] != -1:
                print("mag X data", data[MAG_STATE_X_INDEX])
            if data[MAG_STATE_Y_INDEX] != -1:
                print("mag Y data", data[MAG_STATE_Y_INDEX])
            if data[MAG_STATE_Z_INDEX] != -1:
                print("mag Z data", data[MAG_STATE_Z_INDEX])
            print("--------------------")
            flagDataChange = False


class NotificationThread(threading.Thread):
    def __init__(self):
        super(NotificationThread, self).__init__()
        self._stop_flag = threading.Event()
 
    def stop(self):
        self._stop_flag.set()
 
    def active(self):
        return not self._stop_flag.isSet()
 
    def run(self):
        global dev
        while self.active():
            if dev.waitForNotifications(1.5):
                # handleNotification() was called
                continue
            

def animate(i, xs:list, ys:list):
    # Set up x-axis as time axis
    xs.append(time.time()*1000 - start_ms)
    xs = xs[-100:]
    global data
    # Draw all data
    k_values = data[2:5]
    for i in range(3):
        ys[i].append(k_values[i])
        ys[i] = ys[i][-100:]
        # Plot
        ax = axs[i]
        ax.clear()
        ax.set_xlabel('time(ms)')
        ax.plot(xs, ys[i])

scanner = Scanner().withDelegate(ScanDelegate())
devices = scanner.scan(10.0)
n=0
addr = []
deviceIdx = 0
for dev in devices:
    print ("%d: Device %s (%s), RSSI=%d dB" % (n, dev.addr,dev.addrType, dev.rssi))
    addr.append(dev.addr)
    for (adtype, desc, value) in dev.getScanData():
        print (" %s = %s" % (desc, value))
        if value == "IOT32":
            deviceIdx = n
    n += 1
    
print ('Device', deviceIdx)
print (addr[deviceIdx])


print ("Connecting...")
dev = Peripheral(addr[deviceIdx], 'random')
dev.setDelegate(ScanDelegate())


print ("Services...")
for svc in dev.services:
    print (str(svc))
    print(svc.uuid)


try:
    for serviceUUID in serviceList:
        service = dev.getServiceByUUID(UUID(serviceUUID)) 
        for ch in service.getCharacteristics():
            print (str(ch))
            dev.writeCharacteristic(ch.valHandle+1, b"\x01\x00")
            descriptorList = dev.getDescriptors()
            print("====================")
            if ch.uuid == UUID(chUUIDs[HEART_RATE_INDEX]):
                print("heart rate ch handle:", ch.valHandle) 
                chHandles[HEART_RATE_INDEX]= ch.valHandle
            if ch.uuid == UUID(0x2a38):
                print("body location ch handle:", ch.valHandle) 
                bodyLocationValue = int.from_bytes(ch.read(), "big")
                print("body location value:", BODY_LOCATION[bodyLocationValue]) 
            if ch.uuid == UUID(chUUIDs[BUTTON_STATE_INDEX]):
                print("button ch handle:", ch.valHandle) 
                chHandles[BUTTON_STATE_INDEX]= ch.valHandle

            if ch.uuid == UUID(chUUIDs[MAG_STATE_X_INDEX]):
                print("mag X ch handle:", ch.valHandle) 
                chHandles[MAG_STATE_X_INDEX]= ch.valHandle
            if ch.uuid == UUID(chUUIDs[MAG_STATE_Y_INDEX]):
                print("mag X ch handle:", ch.valHandle) 
                chHandles[MAG_STATE_Y_INDEX]= ch.valHandle
            if ch.uuid == UUID(chUUIDs[MAG_STATE_Z_INDEX]):
                print("mag X ch handle:", ch.valHandle) 
                chHandles[MAG_STATE_Z_INDEX]= ch.valHandle
            print("====================")

except Exception as e:
    print("exception:", e)
    dev.disconnect()


notification_thread = NotificationThread()
notification_thread.start()

# Set up animation
time_axis = []
ys = [[] for _ in range(3)]
fig, axs = plt.subplots(1, 3)
start_ms = time.time()*1000
fig.tight_layout(pad=4.0)
f = "./result_animation.mp4" 


ani = animation.FuncAnimation(fig, animate, fargs=(time_axis, ys), interval=100)
ani.save(f)

notification_thread.stop()
notification_thread.join()
