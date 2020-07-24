# MKR1010OccupancyAlert

This is the code for an Arduino MKR1010 project that displays an alert when a store or other location is crowded. This might allow at-risk individuals
to decide whether to enter the store or whether to come back later. It is part of a competition entry that you can find here: https://www.hackster.io/ianmercer/small-store-congestion-warning-742ea4

There are many other possible applications for this technology in home automation, museum space management, real-estate and others, some of these are discussed below.

What's remarkable about this project is that it does not need any external sensors: no motion sensors, cameras, beams or other physical sensors are used. Instead, it works by counting cell phones (provided they have their Bluetooth turned on). Given how hard it is to turn Bluetooth off and keep it off on an iPhone these days I think that the successful detection rate will be very high.

BUT there's a huge problem tracking cell phones using Bluetooth: phone manufacturers are trying to prevent it. They do this by randomizing the bluetooth mac address and by switching mac addresses periodically (and that period is random too). So even during one short visit a single iPhone might switch mac addresses ten times and a naive counting algorithm will count 10 visitors. Here's what that looks like in practice: each color is a different mac address:

![image](https://user-images.githubusercontent.com/347540/88349052-e17ec900-cd03-11ea-8544-eead1d5741e5.png)

Now put 20 cell phones into the same space, each switching MAC addresses at different random intervals and it becomes an indecipherable mess:

![image](https://user-images.githubusercontent.com/347540/88349062-e5aae680-cd03-11ea-97b1-b0fbfc39ba22.png)

This project solves that problem! Whilst the mac address randomization is random it isn't synchonized across devices and so, any two devices that switch will have time ranges where they are overlapping whereas a single device that switches will never overlap in time. For example, if you saw the sequence of mac addresses: A, A, A, B, B, B it could be one cell phone, but A, A, B, A, B, B cannot be one cellphone.

The algorithm that achieves this is similar to the algorithm used to layout events on a calendar display: events that overlap in time end up in separate columns.

Here for example thirty different mac addresses were observed over a period of time, each was observed between 1 and 34 times. Using the packing algorithm these can be condensed into 11 columns such that there is no overlap in any column:

![image](https://user-images.githubusercontent.com/347540/88349046-dcba1500-cd03-11ea-865f-9e657a719214.png)

This means that there are between 11 and 30 distinct devices present, but as you can see, it's highly likely that the true number is 11, maybe 12, but certainly not 30.

# Running the code
You can try the code out on any Arduino MKR1010 without any extra hardware and then add some warning LEDs or other displays later. Simply download the INO file, open it in the Arduino IDE and flash it to your MKR1010. Now open the serial console and watch the log as it figures out how many devices are present:

![image](https://user-images.githubusercontent.com/347540/88352812-f82b1d00-cd0f-11ea-908b-6f0d4d9bd06e.png)

# Hardware
The minimal hardware to make this into a functioning warning light system is a pair of LEDs: one red, one green wired to PIN6 and PIN7:

![image](https://user-images.githubusercontent.com/347540/88349011-c9a74500-cd03-11ea-8a05-90814e02eb4e.png)

I recommend adding a box, connectors and a CAT5 cable so that the sensing element (the MKR1010 board) and the display element (the two LEDs) can be placed farther apart in the store: the MKR1010 close to the middle of the store and the display by the door.

![image](https://user-images.githubusercontent.com/347540/88349068-e9d70400-cd03-11ea-849d-dcf66f0d771c.png)

![image](https://user-images.githubusercontent.com/347540/88349083-f196a880-cd03-11ea-9625-8e37544b276f.png)

The end result might look something like this: a colored LED and a message to people considering entering the space:
![image](https://user-images.githubusercontent.com/347540/88349033-d4fa7080-cd03-11ea-8b79-79eb35d8a785.png)

# Customization
You can easily customize the code to, for example:
* Adjust the detection range with an RSSI limit.
* Adjust the device counts at which it changes colors.
* Adjust the dwell-time before it decides that a device has departed.
* Make the pulsing brighter as the number of devices increases.
* Use higher-powered LEDs with a transistor for each.
* Use a servo and make an analog-clock-style pointer display that displays the risk in a more analog fashion.

# Possibilities
The possibilities for a system based on this code are endless, here are some suggestions for how you might extend it or what you might build using it:

* Add a Smartphone app that talks to the MKR1010 during setup to adjust the customizable values.
* Create a mesh network and cover a larger area with multiple scanners.
* Report statistics back to a web site (would need to turn off Bluetooth, turn on WiFi and then switch back as currently the MKR1010 doesn't seem to support both simultaneously). Below, for example is a plot of activity by time of day by range by number of devices. This could be invaluable extra data for a small store owner trying to figure out staffing levels during these rapidly changing times.

![image](https://user-images.githubusercontent.com/347540/88349088-f8252000-cd03-11ea-92b2-26c02d5b8f8d.png)


# Contact Me
If you are interested in taking this further please contact me on Twitter, in the issues section here, or by email.
