import serial
import threading
import json
import time
import plotly.graph_objs as go
from plotly.subplots import make_subplots


class SerialCommunication:
    def __init__(self, port='COM5', baudrate=115200):
        self.serial = serial.Serial(port, baudrate)
        self.receive_thread = threading.Thread(target=self.receive_message_thread, daemon=True)
        self.stop_receive_thread = False
        self.receive_thread.start()
        self.data = []

    def send_message(self, message):
        message_bytes = bytes(message, 'utf-8')
        self.serial.write(message_bytes)

    def receive_message_thread(self):
        while not self.stop_receive_thread:
            if self.serial.in_waiting > 0:
                message = self.serial.readline().decode('utf-8').rstrip()
                if message[0] == '{' and message[-1] == '}':
                    data = json.loads(message)
                    print('Received message: ', data)
                    if 'sensor1' in data:
                        self.data.append((time.time(), data['sensor1']))

    def get_data(self):
        return self.data

    def close(self):
        self.stop_receive_thread = True
        self.receive_thread.join()
        self.serial.close()


class FigureHandler:
    def __init__(self, data):
        self.x_data = []
        self.y_data = []
        self.fig = make_subplots(rows=1, cols=1)
        self.fig.add_trace(go.Scatter(x=self.x_data, y=self.y_data, name='Sensor Data'), row=1, col=1)
        self.fig.update_layout(title='Sensor Data', xaxis_title='Time', yaxis_title='Value')
        self.data = data

    def update_plot(self):
        self.x_data = [d[0] for d in self.data]
        self.y_data = [d[1] for d in self.data]
        self.fig.update_traces(x=self.x_data, y=self.y_data)

    def show_plot(self):
        self.fig.show()


if __name__ == "__main__":
    # Create SerialCommunication and FigureHandler objects
    ser = SerialCommunication()
    fig = FigureHandler(ser.get_data())

    try:
        while True:
            fig.update_plot()
            time.sleep(1)

    except KeyboardInterrupt:
        print('Exiting...')
        ser.close()

    # Show the plot after the loop
    fig.show_plot()
