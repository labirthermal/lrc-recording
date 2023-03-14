% load the library
loadlibrary('LRCRecording', 'LRCRecordingWrapper.h')

% open a recording
if calllib('LRCRecording', 'Open', 'recording 001.lrc') ~= 0
    error('Could not open the recording.')
end

% show information about the recording
timestamp = calllib('LRCRecording', 'GetFrameTimestamp');
time = datetime(timestamp / 1000, 'ConvertFrom', 'posixtime', 'TimeZone', 'local');
fprintf('date and time %s\n', time)

width = calllib('LRCRecording', 'GetWidth');
height = calllib('LRCRecording', 'GetHeight');
fprintf('resolution %dx%d\n', width, height)

frameCount = calllib('LRCRecording', 'GetFrameCount');
duration = calllib('LRCRecording', 'GetDuration');
fprintf('%d frames, %g s\n', frameCount, duration / 1000)

recNoteLength = calllib('LRCRecording', 'GetRecordingNoteLength');
tmp = repmat(' ', 1, recNoteLength);
recNotePtr = libpointer('stringPtr', tmp);
recNote = calllib('LRCRecording', 'GetRecordingNote', recNotePtr, recNoteLength);
if size(recNote) > 0
    fprintf('Recording note: %s\n', recNote)
end

objectParamsPtr = calllib('LRCRecording', 'GetObjectParameters');
objectParams = libstruct('ObjectParameters', objectParamsPtr);
fprintf('emissivity %g, reflected temperature %g C, ...\n', objectParams.emissivity, objectParams.reflectedTempK - 273.15)

% get the pointer to the temperature data
dataPtr = calllib('LRCRecording', 'GetTemperatureData');
setdatatype(dataPtr, 'singlePtr', width, height)
% get the pointer to the temperature data range
rangePtr = calllib('LRCRecording', 'GetDataRange');
range = libstruct('Range', rangePtr);

% allocate memory for reading frame notes
frameNoteLength = calllib('LRCRecording', 'GetFrameNoteLength');
tmp = repmat(' ', 1, frameNoteLength);
frameNotePtr = libpointer('stringPtr', tmp);

for i = 1:min(3, frameCount)
    % frame index and time
    index = calllib('LRCRecording', 'GetFrameIndex');
    timestamp = calllib('LRCRecording', 'GetFrameTimestamp');
    time = datetime(timestamp / 1000, 'ConvertFrom', 'posixtime', 'TimeZone', 'local');
    timeSec = calllib('LRCRecording', 'GetTimeFromBeginning');
    fprintf('frame %d, time %s, %g s\n', index, time, timeSec / 1000)

    % temperature data (the pointer is still the same)
    figure
    imshow(dataPtr.Value', [range.min, range.max]) % transpose the matrix
    title(sprintf('Frame %d', index))
    % data range (the pointer is still the same)
    fprintf('temperature range %g-%g C\n', range.min, range.max)
    
    % note associated with the frame
    frameNote = calllib('LRCRecording', 'GetFrameNote', frameNotePtr, frameNoteLength);
    if size(frameNote) > 0
        fprintf('note: %s\n', frameNote)
    end
    
    % advance to the next frame
    % (NextFrame does nothing at the end of recording)
    calllib('LRCRecording', 'NextFrame')
    % other available functions: PreviousFrame, Seek(index), Rewind
end

% close the recording
calllib('LRCRecording', 'Close')

% unload the library
clear recNotePtr objectParams objectParamsPtr range rangePtr dataPtr recNotePtr
unloadlibrary('LRCRecording')
