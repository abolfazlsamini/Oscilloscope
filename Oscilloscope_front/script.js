// Initialize the chart
const chartDom = document.getElementById("chart");
const myChart = echarts.init(chartDom, "dark");

// Initial data
let data = [];
let data_mv = [];
let labels = [];

// Chart configuration
const option = {
  title: {
    text: "ESP-32 Oscilloscope",
    left: "left",
  },

  tooltip: {
    trigger: "axis",
    axisPointer: {
      type: "line",
    },
  },

  dataZoom: [
    {
      type: "inside",
      start: 0,
      end: 100,
    },
    {
      start: 0,
      end: 100,
    },
  ],
  xAxis: {
    type: "category",
    data: labels,
  },
  yAxis: {
    type: "value",
    name: "mv",
    min: 0,
    max: 5000,
    axisLabel: {
      formatter: "{value} mv",
    },
  },
  series: [
    {
      name: "raw",
      type: "line",

      data: data,
      itemStyle: {
        color: "#5470C6",
      },
      lineStyle: {
        width: 3,
      },
      symbol: "none",
    },
    // {
    //   name: "mV",
    //   type: "line",

    //   data: data_mv,
    //   itemStyle: {
    //     color: "#547006",
    //   },
    //   lineStyle: {
    //     width: 3,
    //   },
    //   symbol: "none", 
    // },
  ],
  animation: false, // Disable animation for better performance
};

// Set initial chart options
myChart.setOption(option);
const pausebtn = document.getElementById("pause");

// Function to update chart data by appending new values
function addData(time, raw_value, voltage_mV) {
  if (pausebtn.checked) return;
  // Add new data point
  data.push(raw_value);
  data_mv.push(voltage_mV);
  labels.push(time);

  const maxDataPoints = 5000;
  if (data.length > maxDataPoints) {
    data.shift(); // Remove the oldest data point
    data_mv.push();
    labels.shift(); // Remove the oldest label
  }
}
// Function to update the chart
function updateChart() {
  myChart.setOption({
    xAxis: {
      data: labels,
    },
    series: [
      {
        data: data,
      },
      {
        data: data_mv,
      },
    ],
  }); // Use notMerge to replace data instead of merging
}

// Throttle chart updates to reduce rendering frequency
let isUpdating = false;
function throttleUpdate() {
  if (!isUpdating) {
    isUpdating = true;
    requestAnimationFrame(() => {
      updateChart();
      isUpdating = false;
    });
  }
}

// WebSocket connection
const ws = new WebSocket("ws://localhost:8080");

ws.onmessage = (event) => {
  let message;

  // Check if the data is a Blob
  if (event.data instanceof Blob) {
    // Convert Blob to text
    const reader = new FileReader();
    reader.onload = () => {
      message = JSON.parse(reader.result); // Parse the JSON string
      addData(message.time, message.raw_value, message.voltage_mV);
      throttleUpdate(); // Throttle chart updates
    };
    reader.readAsText(event.data); // Read the Blob as text
  } else {
    // If it's already text, parse it directly
    message = JSON.parse(event.data);
    addData(message.time, message.raw_value, message.voltage_mV);
    throttleUpdate(); // Throttle chart updates
  }
};

// Handle window resize for responsiveness
window.addEventListener("resize", () => {
  myChart.resize();
});
