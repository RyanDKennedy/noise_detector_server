#!/usr/bin/python3

import sys, os;
import time;
from noise_detector import NoiseDetectorConnection;

HOST = "127.0.0.1";
PORT_NUM = 2255;

def main():

    conn = NoiseDetectorConnection(HOST, PORT_NUM);

    status = conn.set_threshold(10);
    if (status == -1):
        print("failed to set threshold");        
        conn.close();
        sys.exit(1);

    threshold = conn.get_threshold();
    if (threshold == -1):
        print("failed to get threshold");
        conn.close();
        sys.exit(1);
    
    print("threshold: {}".format(threshold));

    status = conn.send_test_msg("How are you doing?");
    if (status == -1):
        print("failed to send test msg");
        conn.close();
        sys.exit(1);
    
    
    conn.close();

    sys.exit(0);
    
if (__name__ == "__main__"):
    main();

