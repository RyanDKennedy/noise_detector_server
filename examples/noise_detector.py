"""
WARNING: DO NOT RUN THIS ON MULTIPLE THREADS AT A TIME
"""

import socket;

REQUEST_TYPE_EXIT                   = 0
REQUEST_TYPE_SET_THRESHOLD          = 1
REQUEST_TYPE_GET_THRESHOLD          = 2
REQUEST_TYPE_GET_TIME_OF_LAST_ALARM = 3
REQUEST_TYPE_TEST_PRINT             = 4

REQUEST_OPTION_GLOBAL_ACK = 1
REQUEST_OPTION_GET_THRESHOLD_RETURN = 1 << 1


class NoiseDetectorConnection:

    def __init__(self, host_ip: str, host_port: int):
        self.host = host_ip;
        self.port = host_port;
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM);
        self.sock.connect( (self.host, self.port) );

    # returns -1 if error, positive or zero if not error
    def get_threshold(self) -> int:
        self.sock.sendall(self.encode_packet(REQUEST_TYPE_GET_THRESHOLD, 0, ""));

        # get ACK
        packet = self.sock.recv(3);
        (req_size, req_type, req_opts, data) = self.decode_packet(packet);
        if not (req_type == REQUEST_TYPE_GET_THRESHOLD and req_opts & REQUEST_OPTION_GLOBAL_ACK):
            return -1;

        # get value
        packet = self.sock.recv(255);
        (req_size, req_type, req_opts, data) = self.decode_packet(packet);
        if (req_type == REQUEST_TYPE_GET_THRESHOLD and req_opts & REQUEST_OPTION_GET_THRESHOLD_RETURN):
            return int(data);
        else:
            return -1;
        
    # returns -1 if error, zero if not error
    def set_threshold(self, value: int) -> int:
        self.sock.sendall(self.encode_packet(REQUEST_TYPE_SET_THRESHOLD, 0, str(value)));

        # get ACK
        packet = self.sock.recv(3);
        (req_size, req_type, req_opts, data) = self.decode_packet(packet);
        if not (req_type == REQUEST_TYPE_SET_THRESHOLD and req_opts & REQUEST_OPTION_GLOBAL_ACK):
            return -1;

        return 0;

        
    # returns -1 if error, zero if not error
    def close(self):
        self.sock.sendall(self.encode_packet(REQUEST_TYPE_EXIT, 0, ""));

        # get ACK
        packet = self.sock.recv(3);
        (req_size, req_type, req_opts, data) = self.decode_packet(packet);
        if not (req_type == REQUEST_TYPE_EXIT and req_opts & REQUEST_OPTION_GLOBAL_ACK):
            return -1;

        self.sock.close();

        return 0;

    # returns -1 if error, zero if not error
    def send_test_msg(self, str) -> int:
        self.sock.sendall(self.encode_packet(REQUEST_TYPE_TEST_PRINT, 0, str));

        # get ACK
        packet = self.sock.recv(3);
        (req_size, req_type, req_opts, data) = self.decode_packet(packet);
        if not (req_type == REQUEST_TYPE_TEST_PRINT and req_opts & REQUEST_OPTION_GLOBAL_ACK):
            return -1;

        return 0;


    def encode_packet(self, req_type: int, req_opts: int, data: str) -> bytearray:
        data_len = len(data);
        req_size = 3 + data_len;

        if (req_size > 255):
            req_size = 255;
            data_len = 252;
       
        header = bytearray([req_size, req_type, req_opts]);
        return header + bytearray(data[0:data_len], "ascii");

    # returns (req_size, req_type, req_opts, data)
    def decode_packet(self, packet: bytearray) -> (int, int, int, str):
        req_size: int = int(packet[0]);
        req_type: int = int(packet[1]);
        req_opts: int = int(packet[2]);
        data: str = packet[3:].decode("ascii");
        return (req_size, req_type, req_opts, data);
