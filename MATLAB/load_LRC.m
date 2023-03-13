function [frames,time_axes] = load_LRC(filename)
%Function uses LRCRecording library to load sequence directly to
% matlab. load_LRC.m, LRCRecording.dll, LRCRecordingWrapper.h 
% and DataStructures.h has to be in the same folder
%
% frames contains the whole sequence
% time_axes contains time axes - second from the beginning
%
% usage: [frames,t]=load_LRC('recording 001.lrc');

warning('off','all')

loadlibrary('LRCRecording', 'LRCRecordingWrapper.h')

% open a recording
if calllib('LRCRecording', 'Open', filename) ~= 0
    error('Could not open the recording.')
end

frameCount = calllib('LRCRecording', 'GetFrameCount');

width = calllib('LRCRecording', 'GetWidth');
height = calllib('LRCRecording', 'GetHeight');

frames=zeros(height,width,frameCount);
time_axes=zeros(frameCount,1);

dataPtr = calllib('LRCRecording', 'GetTemperatureData');
setdatatype(dataPtr, 'singlePtr', width, height)

for i = 1:frameCount
    % frame index and time
    timeSec = calllib('LRCRecording', 'GetTimeFromBeginning');
    
    time_axes(i)=timeSec / 1000;
    
    % temperature data (the pointer is still the same)
    frames(:,:,i)=dataPtr.Value';
    
    
    % advance to the next frame
    % (NextFrame does nothing at the end of recording)
    calllib('LRCRecording', 'NextFrame')
end

% close the recording
calllib('LRCRecording', 'Close')

clear dataPtr

unloadlibrary('LRCRecording')

warning('on','all')
end

