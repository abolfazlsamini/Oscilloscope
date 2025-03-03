# Oscilloscope using ESP-32 over localnetwork (websocket) in esp-idf vscode

It's a very simple project to visualize an analog input from an ESP-32.\
It uses [ECharts](https://echarts.apache.org/en/index.html) as the front-end to visualize the input.

## This is how it looks like:
![video](https://github.com/user-attachments/assets/ad131b42-8035-47d3-8aea-270ea8bce11f)
![front](https://github.com/user-attachments/assets/38312ce3-7039-4918-aed3-e222d7c40a6c)

## How to use it:
(this part is not necessary if you flash your esp using other methods)
First you need vs-code and the esp-idf extension installed more about that in [here](https://docs.espressif.com/projects/esp-idf/en/v4.4.3/esp32/get-started/index.html#introduction)\
\
Then you need to clone this repository
```
git clone https://github.com/abolfazlsamini/Oscilloscope.git
```
\
Open project with vs-code. Build and flash your esp-32\
After that, go to Oscilloscope_front folder and open terminal and install dependencies (make sure you have npm installed from [here](https://nodejs.org/en/download)
```
npm i
```
then run server
```
node server.js
```
now open index.html in your browser (i suggest you don't use live-server because it could interfere with web socket connection of esp-32)
That's it.😊👍
