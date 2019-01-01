% 
%       __            ____
%      / /__ _  __   / __/                      __  
%     / //_/(_)/ /_ / /  ___   ____ ___  __ __ / /_ 
%    / ,<  / // __/_\ \ / _ \ / __// _ \/ // // __/ 
%   /_/|_|/_/ \__//___// .__//_/   \___/\_,_/ \__/  
%                     /_/   github.com/KitSprout    
%  
%  @file    kSerial.m
%  @author  KitSprout
%  @date    01-JAN-2019
%  @brief   
% 

classdef kSerial < handle

properties (SetAccess = public)
    ks = struct;
end

properties (SetAccess = private)
    serial;
    comPort = '';
    baudRate = 115200;
    inputBufferSize = 8192;
    outputBufferSize = 8192;

    tick   = struct;
    record = struct;
    packet = struct;
end

methods

    % ---- constructor
    % s = kSerial()                           ->  port = 'auto', baudRate = 115200, no delete instrfindall
    % s = kSerial(baudRate)                   ->  port = 'auto', set baudRate, no delete instrfindall
    % s = kSerial(port, baudRate)             ->  set port, set baudRate, no delete instrfindall
    % s = kSerial(baudRate, 'clear')          ->  port = 'auto', delete instrfindall
    % s = kSerial(port, baudRate, 'clear')    ->  set port, set baudRate, delete instrfindall
    %                                      	  ->  port = 'COMx', 'auto', 'select'
    function s = kSerial( varargin )
        switch nargin
            case 0
                port = 'auto';
                s.baudRate = 115200;
            case 1
                port = 'auto';
                s.baudRate = varargin{1};
            case 2
                if ischar(varargin{1})
                    port = varargin{1};
                    s.baudRate = varargin{2};
                else
                    port = 'auto';
                    s.baudRate = varargin{1};
                    if strcmp(varargin{2}, 'clear')
                        delete(instrfindall);
                    end
                end
            case 3
                port = varargin{1};
                s.baudRate = varargin{2};
                if strcmp(varargin{3}, 'clear')
                    delete(instrfindall);
                end
            otherwise
                error('input error!!');
        end
        info = instrhwinfo('serial');
        comPortList = info.AvailableSerialPorts;

        if strcmp(port, 'auto')
            s.comPort = char(comPortList(1));
        elseif strcmp(port, 'select')
            fprintf('-- serial port : ');
            for i = 1 : size(char(comPortList), 1)
                fprintf(['\t[%d] ', char(comPortList(i))], i);
            end
            comPort = input('\n');
            if isempty(comPort) || (comPort > size(char(comPortList), 1))
                s.comPort = comPortList(1);
            else
                s.comPort = comPortList(comPort);
            end
        elseif strncmp(port, 'COM', 3)
            s.comPort = port;
        else
            error('com port error!!');
        end

        s.serial = serial(s.comPort, 'BaudRate', s.baudRate, 'DataBits', 8, 'StopBits', 1, 'Parity', 'none', 'FlowControl', 'none');
        s.serial.ReadAsyncMode = 'continuous';
        s.serial.InputBufferSize = s.inputBufferSize;
        s.serial.OutputBufferSize = s.outputBufferSize;

        fprintf(['com port : ', s.comPort, '\n']);

        s.packet.maxLens = 1;

        s.packet.recvThreshold = 0;
        s.packet.recvBytes     = 0;
        s.packet.recvBuffer    = [];
        s.packet.recvBuffSize  = 0;

        s.record.info       = [];
        s.record.data       = [];
        s.record.bufferSize = 0;
        s.record.custdata   = [];
        s.record.custlens   = 0;
        s.record.count      = 0;
        s.record.totalBytes = 0;

        s.ks.info  = [];
        s.ks.data  = [];
        s.ks.lens  = 0;

        s.tick.state  = 0;
        s.tick.time   = 0;
        s.tick.count  = s.record.count;
        s.tick.dt     = 0;
        s.tick.alpha  = 0;
        s.tick.freq   = 0;
        s.tick.period = 0.5;

        tic
    end

    function open( s )
        fopen(s.serial);
    end

    function close( s )
        fclose(s.serial);
    end

    function data = read( s, bytes, type )
        data = fread(s.serial, bytes, type);
    end

    function bytes = write( s, data, type )
        fwrite(s.serial, data, type);
        bytes = length(data);
    end

    function clear( s )
        flushinput(s.serial);
    end

    function delay( ~, delay )
        pause(delay);
    end

    function setBaudRate( s, baudRate )
        s.baudRate = baudRate;
        s.serial.baudRate = s.baudRate;
    end

    function setInputBufferSize( s, bufferSize )
        s.inputBufferSize = bufferSize;
        s.serial.InputBufferSize = s.inputBufferSize;
    end

    function setOutputBufferSize( s, bufferSize )
        s.outputBufferSize = bufferSize;
        s.serial.OutputBufferSize = s.outputBufferSize;
    end

    function setCustomizeBuffer( s, customizeLens )
        s.record.custlens = customizeLens;
        s.record.custdata = zeros(s.record.custlens, s.record.bufferSize);
    end

    function updateCustomizeData( s, data )
        [tp, lens] = size(data);
        s.record.custdata(:, end - lens + 1 : end) = data;
        s.ks.data(end - tp + 1 : end, end - lens + 1 : end) = data;
    end

    function setRecordBufferSize( s, bufferSize )
        s.record.bufferSize = bufferSize;
        s.record.info = zeros(5, s.record.bufferSize);
        s.record.data = zeros(s.packet.maxLens, s.record.bufferSize);
    end

    function setRecvThreshold( s, threshold )
        s.packet.recvThreshold = threshold;
    end

    % [data, info, count] = s.packetRecv();
    % info(1) : length
    % info(2) : data type
    % info(3) : parameter 1
    % info(4) : parameter 2
    % info(5) : checksum
    function varargout = packetRecv( s )

        % default value
        s.packet.info = [];
        s.packet.data = [];
        s.packet.availableCount = [];
        s.packet.availableIndex = [];

        % start to read
        bytes = get(s.serial, 'BytesAvailable');
        if bytes > s.packet.recvThreshold
            s.packet.recvBytes = fread(s.serial, bytes, 'uint8');
            s.packet.recvBuffer = [s.packet.recvBuffer; s.packet.recvBytes];
            s.packet.recvBuffSize = size(s.packet.recvBuffer, 1);

            % find available packet
            packetIndex = find(s.packet.recvBuffer == 75);  % 'K'
            % ignore incomplete packet
            if  ~isempty(packetIndex) && packetIndex(end) > s.packet.recvBuffSize - 7
                packetIndex(end) = [];
            end
            if ~isempty(packetIndex)
                % check packet hrader
                subIndex = find(s.packet.recvBuffer(packetIndex + 1) == 83);  % 'S'
                if ~isempty(subIndex)
                    packetIndex = packetIndex(subIndex);  % 'KS' index
                    packetLens  = s.packet.recvBuffer(packetIndex + 2);  % 10-bit packet length
                    packetLens  = packetLens + fix(s.packet.recvBuffer(packetIndex + 3) / 64) * 256;

                    % check packet data length
                    subIndex = find(packetLens >= 8);
                    if ~isempty(subIndex)
                        packetIndex = packetIndex(subIndex);  % 'KS' and lens index
                        packetLens  = packetLens(subIndex);
                        for i = size(packetIndex) : -1 : 1
                            if  packetIndex(i) + packetLens(i) - 1 > s.packet.recvBuffSize
                                packetIndex(i) = [];
                                packetLens(i)  = [];
                            else
                                % check checksum
                                checksum = s.getChecksum(s.packet.recvBuffer(packetIndex(i) : packetIndex(i) + 5));
                                if checksum ~= s.packet.recvBuffer(packetIndex(i) + 6)
                                    packetIndex(i) = [];
                                    packetLens(i)  = [];
                                end
                            end
                        end

                        % check finish signal
                        subIndex = find(s.packet.recvBuffer(packetIndex + packetLens - 1) == 13);   % '\r'
                        if ~isempty(subIndex)
                            s.packet.availableIndex = packetIndex(subIndex);  % 'KS', lens and '\r' index
                            s.packet.availableCount = size(s.packet.availableIndex, 1);

                            % get available packet information and data
                            s.packet.data = zeros(s.packet.maxLens, s.packet.availableCount);
                            s.packet.info = zeros(5, s.packet.availableCount);

                            s.packet.info(1, :) = packetLens(subIndex)';                                        % length
                            s.packet.info(2, :) = mod(s.packet.recvBuffer(s.packet.availableIndex + 3)', 64);   % data type
                            s.packet.info(3, :) = s.packet.recvBuffer(s.packet.availableIndex + 4)';            % parameter 1
                            s.packet.info(4, :) = s.packet.recvBuffer(s.packet.availableIndex + 5)';            % parameter 2
                            s.packet.info(5, :) = s.packet.recvBuffer(s.packet.availableIndex + 6)';            % checksum

                            s.record.count = s.record.count + s.packet.availableCount;
                            s.record.totalBytes = s.record.totalBytes + sum(s.packet.info(1, :));

                            for i = 1 : s.packet.availableCount
                                if s.packet.info(1, i) == 8
                                    s.packet.data(:, i) = zeros(s.packet.maxLens, 1);
                                else
                                    idx  = s.packet.availableIndex(i);
                                    typ  = s.typeConv(s.packet.info(2, i));
                                    lens = (s.packet.info(1, i) - 8) / s.typeSize(s.packet.info(2, i));
                                    if lens > s.packet.maxLens
                                        s.packet.data(s.packet.maxLens + 1 : lens, :) = zeros(lens - s.packet.maxLens, size(s.packet.data, 2));
                                        s.packet.maxLens = lens;
                                    end
                                    convdata = typecast(uint8(s.packet.recvBuffer(idx + 7 : idx + s.packet.info(1, i) - 2)), typ);
                                    s.packet.data(:, i) = [convdata; zeros(s.packet.maxLens - size(convdata, 1), 1)];
                                end
                            end

                            % update recv buffer & length
                            leastIndex = s.packet.availableIndex(end) + s.packet.info(1, end) - 1;
                            s.packet.recvBuffer(1 : leastIndex) = [];

                            % save to buffer
                            if size(s.record.data, 1) < s.packet.maxLens
                                s.record.data(end + 1 : s.packet.maxLens, :) = zeros(s.packet.maxLens - size(s.record.data, 1), size(s.record.data, 2));
                            end
                            s.record.data = [s.record.data(:, s.packet.availableCount + 1 : end), s.packet.data];
                            s.record.info = [s.record.info(:, s.packet.availableCount + 1 : end), s.packet.info];

                            s.ks.lens = s.record.count;
                            s.ks.info = s.getRecordInfo();
                            if s.record.custlens > 0
                                s.record.custdata = [s.record.custdata(:, s.packet.availableCount + 1 : end), zeros(s.record.custlens, size(s.packet.data, 2))];
                                s.ks.data = [s.getRecordData(); s.getCustomizeData()];
                            else
                                s.ks.data = s.getRecordData();
                            end
                        end
                    end
                end
            end
        end
        varargout = { s.packet.data, s.packet.info, s.packet.availableCount };
    end

    % bytes = s.packetSend();                   ->  null (8-bit)
    % bytes = s.packetSend(param);              ->  only send parameter (8-bit)
    % bytes = s.packetSend(param, data);        ->  automatic detect data type
    % bytes = s.packetSend(param, data, type);  ->  user define data type
    function varargout = packetSend( s, varargin )
        switch nargin
            case 1
                % bytes = s.packetSend();
                typestr  = 'null';
                type     = s.typeConv('null');
                sendbuff = uint8(['KS', 8, type, 0, 0, 0, 13]);
                sendbuff(7) = s.getChecksum(sendbuff(1:6));
            case 2
                % bytes = s.packetSend(param);
                param    = varargin{1};
                typestr  = 'null';
                type     = s.typeConv('null');
                sendbuff = uint8(['KS', 8, type, param(1), param(2), 0, 13]);
                sendbuff(7) = s.getChecksum(sendbuff(1:6));
            case 3
                % bytes = s.packetSend(param, data);
                param    = varargin{1};
                data     = varargin{2};
                typestr  = class(data);
                [lens, type, data] = s.getTypesAndLens(data, typestr);
                convdata = s.getData2Bytes(data, typestr);
                sendbuff = uint8(['KS', lens, type, param(1), param(2), 0, convdata, 13]);
                sendbuff(7) = s.getChecksum(sendbuff(1:6));
            case 4
                % bytes = s.packetSend(param, data, type);
                param    = varargin{1};
                data     = varargin{2};
                typestr  = varargin{3};
                [lens, type, data] = s.getTypesAndLens(data, typestr);
                convdata = s.getData2Bytes(data, typestr);
                sendbuff = uint8(['KS', lens, type, param(1), param(2), 0, convdata, 13]);
                sendbuff(7) = s.getChecksum(sendbuff(1:6));
        end

        s.write(sendbuff, 'uint8');
        bytes = size(sendbuff, 2);
        type  = s.typeConv(typestr);
        info  = [bytes, type, double(sendbuff(5:7))];

        varargout = { info, sendbuff };
    end

    % [freq, tt] = s.getFreqAndTime(cutoff);                ->  use system clock to calculate
    % [freq, tt] = s.getFreqAndTime(index, unit, cutoff);   ->  use packet sec/msc to calculate
    function varargout = getFreqAndTime( s, varargin )
        switch nargin
            case 2
                % [freq, tt] = s.getFreqAndTime(cutoff);
                tau = varargin{1};
                s.tick.dt = toc;
                if s.tick.state < round(2 / s.tick.period)
                    if s.tick.dt >= s.tick.period
                        tic;
                        s.tick.freq = (s.ks.lens - s.tick.count) / s.tick.dt;
                        s.tick.count = s.ks.lens;
                        s.tick.state = s.tick.state + 1;
                        s.tick.time = s.tick.time + s.tick.dt;
                        s.tick.dt = 0;
                    end
                elseif s.tick.dt >= s.tick.period
                    tic;
                    fq = (s.ks.lens - s.tick.count) / s.tick.dt;
                    s.tick.alpha = tau / (tau + s.tick.dt);
                    s.tick.count = s.ks.lens;
                    s.tick.freq = s.tick.alpha * s.tick.freq + (1 - s.tick.alpha) * fq;
                    s.tick.time = s.tick.time + s.tick.dt;
                    s.tick.dt = 0;
                end
                tims = s.tick.time + s.tick.dt;
                freq = round(s.tick.freq);
            case 4
                % [freq, tt] = s.getFreqAndTime(index, unit, cutoff);
                index = varargin{1};
                unit  = varargin{2};
                tau   = varargin{3};
                tims  = s.ks.data(index(1), end) + s.ks.data(index(2), end) * unit;
                s.tick.dt = tims - s.tick.time;
                if s.tick.state < round(2 / s.tick.period)
                    if s.tick.dt >= s.tick.period
                        s.tick.time = tims;
                        s.tick.freq = (s.ks.lens - s.tick.count) / s.tick.dt;
                        s.tick.count = s.ks.lens;
                        s.tick.state = s.tick.state + 1;
                    end
                elseif s.tick.dt >= s.tick.period
                    s.tick.time = tims;
                    fq = (s.ks.lens - s.tick.count) / s.tick.dt;
                    s.tick.alpha = tau / (tau + s.tick.dt);
                    s.tick.count = s.ks.lens;
                    s.tick.freq = s.tick.alpha * s.tick.freq + (1 - s.tick.alpha) * fq;
                end
                freq = round(s.tick.freq);
        end
        msc = fix((tims - fix(tims)) * 1000 + 1e-5);
        sec = mod(fix(tims), 60);
        min = fix(fix(tims) / 60);
        varargout = {freq, [tims, min, sec, msc]};
    end

    % [rate, lost, dc] = s.getLostRate();                       ->  use parameter bytes to check lost packet
    % [rate, lost, dc] = s.getLostRate(index, unit, freq);      ->  use packet sec/msc to check lost packet
    function varargout = getLostRate( s, varargin )
        switch nargin
            case 1
                count = zeros(1, s.ks.lens);
                for i = 1 : s.ks.lens
                    count(i) = s.getParameter(i, 'uint16');
                end
                dc = count(2:end) - count(1:end-1);
                idx = find(dc < 0);
                dc(idx) = dc(idx) + 65536;
                lost = size(find(dc > 1), 2);
            case 4
                index = varargin{1};
                unit  = varargin{2};
                freq  = varargin{3};
                tc = fix(s.ks.data(index(1), :) / unit) + s.ks.data(index(2), :);
                dc = tc(2 : end) - tc(1 : end - 1);
                res = find(dc > round(1000 / freq));
                lost = size(res, 2);
        end
        rate = lost / s.ks.lens;

        varargout = { rate, lost, dc };
    end

    function data = getCustomizeData( s )
        if s.record.count < s.record.bufferSize
            data = s.record.custdata(:, end - s.record.count + 1 : end);
        else
            data = s.record.custdata;
        end
    end

    function data = getRecordData( s )
        if s.record.count < s.record.bufferSize
            data = s.record.data(:, end - s.record.count + 1 : end);
        else
            data = s.record.data;
        end
    end

    function data = getRecordInfo( s )
        if s.record.count < s.record.bufferSize
            data = s.record.info(:, end - s.record.count + 1 : end);
        else
            data = s.record.info;
        end
    end

    function param = getParameter( s, index, typestr )
        param = typecast(uint8(s.ks.info(3:4, index)'), typestr);
    end

    function filename = save2mat( s, varargin )
        fprintf('\nfilename : ');
        date = fix(clock);
        tag  = sprintf('_%04i%02i%02i_%02i%02i%02i.mat', date);
        name = varargin{1};
        filename = strcat(name, tag);
        fprintf(filename);
      	fprintf('  SAVE... ');

        ks = s.ks;
        save(filename, 'ks');

        switch nargin
            case 3
                sv = varargin{2};
                if ~isempty(sv)
                    save(filename, 'sv', '-append');
                end
        end

        fprintf('OK\n');
    end

	function typeOutput = typeConv( ~, typeInput )
        if isequal(class(typeInput), 'char')
            switch typeInput
                case 'null',     typeOutput = 0;
                case 'int8',     typeOutput = 17;
                case 'int16',    typeOutput = 18;
                case 'int32',    typeOutput = 20;
                case 'int64',    typeOutput = 24;
                case 'uint8',    typeOutput = 33;
                case 'uint16',   typeOutput = 34;
                case 'uint32',   typeOutput = 36;
                case 'uint64',   typeOutput = 40;
                case 'single',   typeOutput = 68;
                case 'double',   typeOutput = 72;
            end
        else
            typeInput = mod(typeInput, 64);
            switch typeInput
                case 0,   typeOutput = 'null';    % 0x00, 8'b 0000 0000
                case 17,  typeOutput = 'int8';    % 0x11, 8'b 0001 0001
                case 18,  typeOutput = 'int16';   % 0x12, 8'b 0001 0010
                case 20,  typeOutput = 'int32';   % 0x14, 8'b 0001 0100
                case 24,  typeOutput = 'int64';   % 0x18, 8'b 0001 1000
                case 33,  typeOutput = 'uint8';   % 0x21, 8'b 0010 0001
                case 34,  typeOutput = 'uint16';  % 0x22, 8'b 0010 0010
                case 36,  typeOutput = 'uint32';  % 0x24, 8'b 0010 0100
                case 40,  typeOutput = 'uint64';  % 0x28, 8'b 0010 1000
                case 68,  typeOutput = 'single';  % 0x44, 8'b 0100 0100
                case 72,  typeOutput = 'double';  % 0x48, 8'b 0100 1000
            end
        end
    end

	function bytes = typeSize( ~, type )
        if isequal(class(type), 'char')
            switch type
                case 'null',    bytes = 0;
                case 'int8',    bytes = 1;
                case 'int16',   bytes = 2;
                case 'int32',   bytes = 4;
                case 'int64',   bytes = 8;
                case 'uint8',   bytes = 1;
                case 'uint16',  bytes = 2;
                case 'uint32',  bytes = 4;
                case 'uint64',  bytes = 8;
                case 'single',  bytes = 4;
                case 'double',  bytes = 8;
            end
        else
            switch type
                case 0,   bytes = 0;  % null
                case 17,  bytes = 1;  % int8
                case 18,  bytes = 2;  % int16
                case 20,  bytes = 4;  % int32
                case 24,  bytes = 8;  % int64
                case 33,  bytes = 1;  % uint8
                case 34,  bytes = 2;  % uint16
                case 36,  bytes = 4;  % uint32
                case 40,  bytes = 8;  % uint64
                case 68,  bytes = 4;  % single
                case 72,  bytes = 8;  % double
            end
        end
    end

    function convdata = getData2Bytes( ~, data, typestr )
        switch typestr
            case 'int8',    convdata = typecast(int8(data),   'uint8');
            case 'int16',   convdata = typecast(int16(data),  'uint8');
            case 'int32',   convdata = typecast(int32(data),  'uint8');
            case 'int64',   convdata = typecast(int64(data),  'uint8');
            case 'uint8',   convdata = typecast(uint8(data),  'uint8');
            case 'uint16',  convdata = typecast(uint16(data), 'uint8');
            case 'uint32',  convdata = typecast(uint32(data), 'uint8');
            case 'uint64',  convdata = typecast(uint64(data), 'uint8');
            case 'single',  convdata = typecast(single(data), 'uint8');
            case 'double',  convdata = typecast(double(data), 'uint8');
        end
    end

    function [lens, type, data] = getTypesAndLens( s, data, typestr )
        [b, i] = max(size(data));
        total  = 8 + s.typeSize(typestr) * b;
        type   = s.typeConv(typestr) + fix(total / 256) * 64;
        lens   = mod(total, 256);
        if i == 1
            data = data(:, 1)';
        elseif i == 2
            data = data(1, :);
        end
    end

    function checksum = getChecksum( ~, packet )
        checksum = mod(sum(packet), 256);
    end
end

methods (Access = private)

end

end
