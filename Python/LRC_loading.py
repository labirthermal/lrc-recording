# -*- coding: utf-8 -*-
"""
Created on Fri Mar 10 10:19:52 2023
The script contains function for loading LRC file with help of LRCRecording.dll

It gives back:
    frames - 3D thermographic sequence
    time_axes - time axes in seconds from the begining of recording 
    
Function call:
    [frames, time_axes]=load_LRC('file_name.lrc')    
    
The DLL library has to be in the same folder as this script
For import to another script you can use from LRC_loading import load_LRC


@author: LukasM
"""

import os
import sys
import ctypes
import ctypes.util
import numpy as np


def load_LRC(file_name):
    """
    Parameters
    ----------
    file_name : string
        Contains whole filename of LRC recording

    Returns
    -------
    frames : 3D nparray of float32
        This is the sequence
    time_axes : array of float32
        Time axes in seconds (from the beginning; start is zero)

    """
    #get the path to LRC recording
    path = os.getcwd()
    mylib_path = os.path.join(path, 'LRCRecording.dll')

    try:
        LRC_lib = ctypes.CDLL(mylib_path)
    except OSError:
        print("Unable to load the system C library")
        sys.exit()

    file_name = file_name.encode('utf-8')

    LRC_lib.Open(ctypes.c_char_p(file_name))  # it takes only ctype string

    frame_count = LRC_lib.GetFrameCount()
    width = LRC_lib.GetWidth()
    height = LRC_lib.GetHeight()

    #definition of pointer for loading of data matrix (2D)
    LRC_lib.GetTemperatureData.restype = ctypes.POINTER(ctypes.c_float)
    pointer_to_frame = ctypes.POINTER(ctypes.c_float)

    #using arrays instead of list for performance
    frames = np.zeros([height, width, frame_count], dtype=np.float32)
    time_axes = np.zeros([frame_count, ], dtype=np.float32)

    for current_frame_number in range(frame_count):

        #get data
        pointer_to_frame = LRC_lib.GetTemperatureData()

        time_axes[current_frame_number] = LRC_lib.GetTimeFromBeginning()/1000

        #get data from pointer.
        one_frame = np.ctypeslib.as_array(
            pointer_to_frame, shape=(height, width))

        frames[:, :, current_frame_number] = one_frame

        #it moves pointer to next frame
        LRC_lib.NextFrame()

    LRC_lib.Close(ctypes.c_char_p(file_name))

    #not sure if there is a memory leak
    del LRC_lib

    return frames, time_axes
