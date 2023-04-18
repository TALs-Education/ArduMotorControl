import serial
import threading
import json
import time
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque

# Define ArduinoCommunication class
class ArduinoCommunication:
    def __init__(self, port='COM5', baudrate=115200, data_filename='sensor_data.txt'):
        self.serial = serial.Serial(port, baudrate)
        self.stop_receive_thread = threading.Event()
        self.receive_thread = threading.Thread(target=self.receive_message_thread, daemon=True)
        self.data_callback = None
        self.receive_thread.start()
        self.data_file = open(data_filename, 'a')

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()

    def send_message(self, message):
        message_bytes = message.encode('utf-8')
        self.serial.write(message_bytes)

    def receive_message_thread(self):
        while not self.stop_receive_thread.is_set():
            if self.serial.in_waiting > 0:
                message = self.serial.readline().decode('utf-8').rstrip()
                try:
                    data = json.loads(message)
                    self.save_to_file(message)
                    if self.data_callback:
                        self.data_callback(data)
                except json.JSONDecodeError:
                    print('Error decoding message:', message)

    def close(self):
        self.stop_receive_thread.set()
        self.receive_thread.join()
        self.serial.close()
        self.data_file.close()

    def save_to_file(self, message):
        self.data_file.write(message + '\n')

# Define SensorDataPlot class
class SensorDataPlot:
    def __init__(self, maxlen=1000):
        self.sensor_data = deque(maxlen=maxlen)
        self.fig, self.ax = plt.subplots()
        self.line, = self.ax.plot([], [], lw=2, label='Sensor 1')

    def handle_data(self, data):
        sensor_value = data.get('sensor1')
        if sensor_value is not None:
            self.sensor_data.append(sensor_value)

    def update_plot(self, num):
        self.line.set_data(range(len(self.sensor_data)), self.sensor_data)
        return self.line,

    def plot(self):
        self.ax.set_xlim(0, self.sensor_data.maxlen)
        self.ax.set_ylim(0, 1023)
        self.ax.set_title('Sensor Data')
        self.ax.set_xlabel('Measurements')
        self.ax.set_ylabel('Sensor Value')
        self.ax.legend(loc='upper right')
        self.ax.grid()  # Add grid to the plot
        ani = animation.FuncAnimation(self.fig, self.update_plot, blit=True, interval=100, repeat=True)
        plt.show()

def main():
    plotter = SensorDataPlot()

    with ArduinoCommunication() as ser:
        ser.data_callback = plotter.handle_data
        plotter.plot()

if __name__ == "__main__":
    main()
