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
%  @date    Dec-2019
%  @brief   kSerial packet format :
%           byte 1   : header 'K' (75)       [HK]
%           byte 2   : header 'S' (83)       [HS]
%           byte 3   : data bytes (12-bit)   [L ]
%           byte 4   : data type             [T ]
%           byte 5   : parameter 1           [P1]
%           byte 6   : parameter 2           [P2]
%           byte 7   : checksum              [CK]
%            ...
%           byte L-1 : data                  [DN]
%           byte L   : finish '\r' (13)      [ER]
% 

classdef kSerial < handle

properties (SetAccess = public)
    ks = struct;
end

properties (SetAccess = private)
    serial;

    recv = struct;
    tick   = struct;
    record = struct;
end

methods

    % ---- constructor
    % s = kSerial()                           ->  port = 'auto', baudrate = 115200, no delete instrfindall
    % s = kSerial(baudrate)                   ->  port = 'auto', set baudrate, no delete instrfindall
    % s = kSerial(port, baudrate)             ->  set port, set baudrate, no delete instrfindall
    % s = kSerial(baudrate, 'clear')          ->  port = 'auto', delete instrfindall
    % s = kSerial(port, baudrate, 'clear')    ->  set port, set baudrate, delete instrfindall
    %                                      	  *** port = 'COMx', 'auto', 'select'
    function s = kSerial( varargin )
        % input setting
        switch nargin
            case 0
                portsel = 'auto';
                baudrate = 115200;
            case 1
                portsel = 'auto';
                baudrate = varargin{1};
            case 2
                if ischar(varargin{1})
                    portsel = varargin{1};
                    baudrate = varargin{2};
                else
                    portsel = 'auto';
                    baudrate = varargin{1};
                    if strcmp(varargin{2}, 'clear')
                        delete(instrfindall);
                    end
                end
            case 3
                portsel = varargin{1};
                baudrate = varargin{2};
                if strcmp(varargin{3}, 'clear')
                    delete(instrfindall);
                end
            otherwise
                error('kserial input error!!');
        end

        % search serial port
        info = instrhwinfo('serial');
        comPortList = info.AvailableSerialPorts;

        if strcmp(portsel, 'auto')
            port = char(comPortList(1));
        elseif strcmp(portsel, 'select')
            for i = 1 : size(char(comPortList), 1)
                fprintf(['\t[%d] ', char(comPortList(i))], i);
            end
            comPort = input(' ... ');
            if isempty(comPort) || (comPort > size(char(comPortList), 1))
                port = char(comPortList(1));
            else
                port = char(comPortList(comPort));
            end
        elseif strncmp(portsel, 'COM', 3)
            port = portsel;
        else
            error('com port error!!');
        end

        % create serial
        s.serial = serial(port, 'BaudRate', baudrate, 'DataBits', 8, 'StopBits', 1, 'Parity', 'none', 'FlowControl', 'none');
        s.serial.ReadAsyncMode = 'continuous';
        s.setInputBufferSize(32 * 1024);
        s.setOutputBufferSize(32 * 1024);

        fprintf(['    com port : ', port, '\n']);

        s.recv.threshold = 0;
        s.recv.buffer = [];
        s.recv.maxlens = 0;
        s.recv.count = 0;

        s.record.buffersize = 0;
        s.record.expandsize = 1024;
        s.record.info = zeros(6, s.record.buffersize);
        s.record.data = zeros(s.recv.maxlens, s.record.buffersize);
        s.record.lens = 0;
        s.record.custdata = [];
        s.record.custsize = 0;

        s.ks.lens  = 0;
        s.ks.info  = [];
        s.ks.data  = [];

        s.tick.state = 0;
        s.tick.time   = 0;
        s.tick.timeunit = 0;
        s.tick.timeindex = [1, 2];
        s.tick.count  = 0;
        s.tick.alpha  = 0.4;
        s.tick.freq   = 0;
        s.tick.period = 0.25;

        % start time tick
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
        bytes = size(data, 2);
    end

    function clear( s )
        flushinput(s.serial);
    end

    function delay( ~, delay )
        pause(delay);
    end

    function setBaudRate( s, baudRate )
        if strcmp(s.serial.Status, 'open')
            fclose(s.serial);
            s.serial.baudRate = baudRate;
            fopen(s.serial);
        else
            s.serial.baudRate = baudRate;
        end
    end

    function setInputBufferSize( s, buffersize )
        if strcmp(s.serial.Status, 'open')
            fclose(s.serial);
            s.serial.InputBufferSize = buffersize;
            fopen(s.serial);
        else
            s.serial.InputBufferSize = buffersize;
        end
    end

    function setOutputBufferSize( s, buffersize )
        if strcmp(s.serial.Status, 'open')
            fclose(s.serial);
            s.serial.OutputBufferSize = buffersize;
            fopen(s.serial);
        else
            s.serial.OutputBufferSize = buffersize;
        end
    end

    % packet = s.pack();                   ->  R0 (8-bit)                    -> [75, 83, 0, 8, 0, 0, 8, 13]
    % packet = s.pack(param);              ->  only send parameter (8-bit)   -> [75, 83, 0, 8, P1, P2, CK, 13]
    % packet = s.pack(param, type);        ->  user define data type (8-bit) -> [75, 83, L, T, P1, P2, CK, 13]
    % packet = s.pack(param, data);        ->  automatic detect data type    -> [75, 83, L, T, P1, P2, Dn, CK, 13]
    % packet = s.pack(param, data, type);  ->  user define data type         -> [75, 83, L, T, P1, P2, Dn, CK, 13]
    function varargout = pack( s, varargin )
        switch nargin
            case 1
                % packet = s.pack();
                packetType = 'R0';
                type = s.typeConv(packetType);
                packet = uint8(['KS', 0, type, 0, 0, 0, 13]);
                packet(7) = s.getChecksum(packet(3:6)');
            case 2
                % packet = s.pack(param);
                param = varargin{1};
                packetType = 'R0';
                type = s.typeConv(packetType);
                packet = uint8(['KS', 0, type, param(1), param(2), 0, 13]);
                packet(7) = s.getChecksum(packet(3:6)');
            case 3
                % packet = s.pack(param, type);
                % packet = s.pack(param, data);
                param = varargin{1};
                if ischar(varargin{2})
                    packetType = varargin{2};
                    type = s.typeConv(packetType);
                    packet = uint8(['KS', 0, type, param(1), param(2), 0, 13]);
                    packet(7) = s.getChecksum(packet(3:6)');
                else
                    data = varargin{2};
                    packetType = class(data);
                    [lens, type, data] = s.getTypesAndLens(data, packetType);
                    convdata = s.getData2Bytes(data, packetType);
                    packet = uint8(['KS', lens, type, param(1), param(2), 0, convdata, 13]);
                    packet(7) = s.getChecksum(packet(3:6)');
                end
            case 4
                % bytes = s.packetSend(param, data, type);
                param = varargin{1};
                data = varargin{2};
                packetType = varargin{3};
                [lens, type, data] = s.getTypesAndLens(data, packetType);
                convdata = s.getData2Bytes(data, packetType);
                packet = uint8(['KS', lens, type, param(1), param(2), 0, convdata, 13]);
                packet(7) = s.getChecksum(packet(3:6)');
        end

        varargout = { packet, packetType };
    end

    % [index, info, data] = s.unpack(packet);
    % info(1) : data length (bytes)
    % info(2) : data type
    % info(3) : parameter 1
    % info(4) : parameter 2
    % info(5) : checksum
    function varargout = unpack( s, packet )
        lens = size(packet, 1);

        % find available packet
        packetIndex = find(packet == 75);  % 'K'
        packetInfo = [];
        packetData = [];

        % ignore incomplete packet
        if  ~isempty(packetIndex)
            packetIndex(packetIndex > lens - 7) = [];
        end
        if ~isempty(packetIndex)
            % check packet hrader
            subIndex = find(packet(packetIndex + 1) == 83);  % 'S'
            if ~isempty(subIndex)
                packetIndex = packetIndex(subIndex);  % 'KS' index
                packetLens  = packet(packetIndex + 2);  % 12-bit data length (byte)
                packetLens  = packetLens + fix(packet(packetIndex + 3) / 16) * 256;

                % check checksum
                checksum = s.getChecksum(packet(packetIndex' + (2:5)'));
                subIndex = find((checksum - packet(packetIndex + 6)) ~= 0);
                if ~isempty(subIndex)
                    packetIndex(subIndex) = [];
                    packetLens(subIndex)  = [];
                end

                % check packet data length
                subIndex = packetIndex + packetLens + 8 - 1 > lens;
                if ~isempty(subIndex)
                    packetIndex(subIndex) = [];
                    packetLens(subIndex)  = [];
                end

                if ~isempty(packetIndex)
                    % check finish signal
                    subIndex = find(packet(packetIndex + packetLens + 8 - 1) == 13);   % '\r'
                    if ~isempty(subIndex)
                        packetIndex = packetIndex(subIndex);
                        packetLens  = packetLens(subIndex);
                        packetInfo  = [packetLens, mod(packet(packetIndex + 3), 16), packet(packetIndex' + (4:6)')']';

                        if all(packetLens == packetLens(1))
                            % same lenght
                            packetData = packet(packetIndex + (7:(packetLens(1)+6)))';
                        else
                            % different length
                            packetData = zeros(max(packetLens), size(packetLens, 1));
                            for i = 1 : size(packetLens, 1)
                                packetData(1:packetLens(i), i) = packet(packetIndex(i) + (7:(packetLens(i)+6)));
                            end
                        end
                    end
                end
            end
        end

        varargout = { packetIndex, packetInfo, packetData };
    end

    % info = s.packetSend();                   ->  null (8-bit)
    % info = s.packetSend(param);              ->  only send parameter (8-bit)
    % info = s.packetSend(param, type);        ->  user define data type (8-bit)
    % info = s.packetSend(param, data);        ->  automatic detect data type
    % info = s.packetSend(param, data, type);  ->  user define data type
    function varargout = packetSend( s, varargin )
        switch nargin
            case 1
                % bytes = s.packetSend();
                [packet, packetType] = s.pack();
            case 2
                % bytes = s.packetSend(param);
                param = varargin{1};
                packetType = 'R0';
                type = s.typeConv(packetType);
                packet = uint8(['KS', 0, type, param(1), param(2), 0, 13]);
                packet(7) = s.getChecksum(packet(3:6)');
            case 3
                % bytes = s.packetSend(param, type);
                % bytes = s.packetSend(param, data);
                param = varargin{1};
                if ischar(varargin{2})
                    packetType = varargin{2};
                    type = s.typeConv(packetType);
                    packet = uint8(['KS', 0, type, param(1), param(2), 0, 13]);
                    packet(7) = s.getChecksum(packet(3:6)');
                else
                    data = varargin{2};
                    packetType = class(data);
                    [lens, type, data] = s.getTypesAndLens(data, packetType);
                    convdata = s.getData2Bytes(data, packetType);
                    packet = uint8(['KS', lens, type, param(1), param(2), 0, convdata, 13]);
                    packet(7) = s.getChecksum(packet(3:6)');
                end
            case 4
                % bytes = s.packetSend(param, data, type);
                param = varargin{1};
                data = varargin{2};
                packetType = varargin{3};
                [lens, type, data] = s.getTypesAndLens(data, packetType);
                convdata = s.getData2Bytes(data, packetType);
                packet = uint8(['KS', lens, type, param(1), param(2), 0, convdata, 13]);
                packet(7) = s.getChecksum(packet(3:6)');
        end

        bytes = s.write(packet, 'uint8') - 8;
        type  = s.typeConv(packetType);
        packetInfo = [bytes, type, double(packet(5:7))]';

        varargout = { packetInfo, double(packet)' };
    end

    % [info, data, count] = s.packetRecv();
    % info(1) : data length (bytes)
    % info(2) : data type
    % info(3) : parameter 1
    % info(4) : parameter 2
    % info(5) : checksum
    % info(6) : packet data length
    function varargout = packetRecv( s, varargin )
        % default value
        packetInfo = [];
        packetData = [];
        packetCount = 0;

        % start to read
        bytes = get(s.serial, 'BytesAvailable');
        if bytes > s.recv.threshold
            recvbytes = fread(s.serial, bytes, 'uint8');
            s.recv.buffer = [s.recv.buffer; recvbytes];
            [packetIndex, packetInfo, packetDataBytes] = s.unpack(s.recv.buffer);
            if ~isempty(packetInfo)
                % clear buffer data
                lastPacketIndex = packetIndex(end) + packetInfo(1, end) + 8 - 1;
                s.recv.buffer(1 : lastPacketIndex) = [];

                % update packet data lenght to packetInfo
                packetCount = size(packetInfo, 2);
              	packetInfo(6, :) = 0;
                for i = 1 : packetCount
                    packetInfo(6, i) = packetInfo(1, i) / s.typeSize(packetInfo(2, i));
                end
                if s.recv.maxlens < max(packetInfo(6, :))
                    s.recv.maxlens = max(packetInfo(6, :));
                end

                if all(packetInfo(2, :) == packetInfo(2, 1))
                    % same type
                    typestr = s.typeConv(packetInfo(2, 1), 'cast');
                    packetData = typecast(uint8(packetDataBytes(:)'), typestr);
                    packetData = reshape(packetData, packetInfo(6, 1), []);
                else
                    % different type
                    packetData = zeros(max(packetInfo(6, :)), packetCount);
                    for i = 1 : packetCount
                        typestr = s.typeConv(packetInfo(2, i), 'cast');
                        packetData(1:packetInfo(6, i), i) = typecast(uint8(packetDataBytes(1:packetInfo(1, i), i)), typestr);
                    end
                end
                packetData = double(packetData);
                s.recv.count = packetCount;

                % record mode
                if (nargin == 2) && strcmp(varargin{1}, 'record')
                    % expand buffer
                    if size(s.record.data, 1) < s.recv.maxlens
                        s.record.data((end + 1) : s.recv.maxlens, :) = 0;
                    elseif (s.record.lens + packetCount) > s.record.buffersize
                        s.record.buffersize = s.record.buffersize + s.record.expandsize;
                        s.record.info = [zeros(size(s.record.info, 1), s.record.expandsize), s.record.info];
                        s.record.data = [zeros(size(s.record.data, 1), s.record.expandsize), s.record.data];
                        if s.record.custsize > 0
                            s.record.custdata = [zeros(s.record.custsize, s.record.expandsize), s.record.custdata];
                        end
                    end

                    % add data to buffer
                    s.record.lens = s.record.lens + s.recv.count;
                    s.record.info = [s.record.info(:, (s.recv.count + 1) : end), packetInfo];
                    s.record.data = [s.record.data(1:max(packetInfo(6, :)), (s.recv.count + 1) : end), packetData];

                    % save to ks
                    s.ks.lens = s.record.lens;
                    s.ks.info = s.getRecordInfo();
                    s.ks.data = s.getRecordData();
                end
            end
        end

        varargout = { packetInfo, packetData, packetCount };
    end

    % data = s.packetSendRecv();                   ->  null (8-bit)
    % data = s.packetSendRecv(param);              ->  only send parameter (8-bit)
    % data = s.packetSendRecv(param, type);        ->  user define data type (8-bit)
    % data = s.packetSendRecv(param, data);        ->  automatic detect data type
    % data = s.packetSendRecv(param, data, type);  ->  user define data type
    function varargout = packetSendRecv( s, varargin )
        timeout = 10000;
        switch nargin
            case 1
                s.packetSend();
            case 2
                s.packetSend(varargin{1});
            case 3
                s.packetSend(varargin{1}, varargin{2});
            case 4
                s.packetSend(varargin{1}, varargin{2}, varargin{3});
        end

        count = 0;
        packetInfo = [];
        while isempty(packetInfo) && count < timeout
            [packetInfo, packetData] = s.packetRecv();
            count = count + 1;
        end

        if count >= timeout
            count = -1;
        end

        varargout = { packetInfo, packetData, count };
    end

    function setPacketObserverWeighting( s, weighting )
        s.tick.alpha = weighting;
    end

    function setPacketObserverTimeunit( s, timeunit )
        s.tick.timeunit = timeunit;
    end

    function setPacketObserverTimeindex( s, timeindex )
        s.tick.timeindex = timeindex;
    end

    % [freq, tims] = s.packetObserver();        ->  calculate freq and run time
    function varargout = packetObserver( s, varargin )
        s.tick.count = s.tick.count + s.recv.count;
        if (s.tick.timeunit == 0) || (s.tick.timeunit >= size(s.ks.data, 1))
            % use system clock to calculate
            caltype = 'system';
            dt = toc;
            freq = s.tick.count / dt;
            tims = s.tick.time + dt;
            if dt >= s.tick.period
                s.tick.count = 0;
                s.tick.time = tims;
                if s.tick.state > (1.5 / s.tick.period)
                    s.tick.freq = (1 - s.tick.alpha) * s.tick.freq + s.tick.alpha * freq;
                else
                    s.tick.freq = freq;
                    s.tick.state = s.tick.state + 1;
                end
                % reset time tick
                tic;
            end
        else
            % use packet sec/msc to calculate
            caltype = 'packet';
            tims = s.ks.data(s.tick.timeindex(1), end) + s.ks.data(s.tick.timeindex(2), end) * s.tick.timeunit;
            dt = tims - s.tick.time;
            freq = s.tick.count / dt;
            if dt >= s.tick.period
                s.tick.count = 0;
                s.tick.time = tims;
                if s.tick.state > (1.0 / s.tick.period)
                    s.tick.freq = (1 - s.tick.alpha) * s.tick.freq + s.tick.alpha * freq;
                else
                    s.tick.freq = freq;
                    s.tick.state = s.tick.state + 1;
                end
            end
        end
        freq = round(s.tick.freq);
        msc = fix((tims - fix(tims)) * 1000 + 1e-5);
        sec = mod(fix(tims), 60);
        min = fix(fix(tims) / 60);
        varargout = {freq, [tims, min, sec, msc], caltype};
    end

    % [rate, lost, dc] = s.getLostRate();       ->  check lost packet
    function varargout = getPacketLostRate( s, varargin )
        if (s.tick.timeunit == 0) || (s.tick.timeunit >= size(s.ks.data, 1))
            % use parameter bytes to check lost packet
            caltype = 'parameter';
            count = zeros(1, s.ks.lens);
            for i = 1 : s.ks.lens
                count(i) = s.getParameter(i, 'uint16');
            end
            dc = count(2:end) - count(1:end-1);
            dc(dc < 0) = dc(dc < 0) + 65536;
            lost = size(find(dc ~= 1), 2);
        else
            % use packet sec/msc to check lost packet
            caltype = 'packet';
            tc = fix(s.ks.data(s.tick.timeindex(1), :) / s.tick.timeunit) + s.ks.data(s.tick.timeindex(2), :);
            dc = tc(2 : end) - tc(1 : end - 1);
            res = find(dc > round(1000 / s.tick.freq));
            lost = size(res, 2);
        end
        rate = lost / (s.ks.lens - 1);

        varargout = { rate, lost, dc, caltype};
    end

    function setRecvThreshold( s, threshold )
        s.recv.threshold = threshold;
    end

    function setRecordBufferSize( s, buffersize )
        s.record.buffersize = buffersize;
        s.record.info = zeros(6, buffersize);
        s.record.data = zeros(s.recv.maxlens, buffersize);
    end

    function setRecordExpandSize( s, expandsize )
        s.record.expandsize = expandsize;
    end

    function info = getRecordInfo( s )
        if s.record.lens < s.record.buffersize
            info = s.record.info(:, (end - s.record.lens + 1) : end);
        else
            info = s.record.info;
        end
    end

    function data = getRecordData( s )
        if s.record.lens < s.record.buffersize
            data = s.record.data(:, (end - s.record.lens + 1) : end);
        else
            data = s.record.data;
        end
    end

    function setCustomizeDataSize( s, custsize )
        s.record.custsize = custsize;
        s.record.custdata = zeros(custsize, s.record.buffersize);
    end

    function data = getCustomizeData( s )
        if s.record.lens < s.record.buffersize
            data = s.record.custdata(:, (end - s.record.lens + 1) : end);
        else
            data = s.record.custdata;
        end
    end

    function updateCustomizeData( s, custdata )
        lens = size(custdata, 2);
        if lens ~= s.recv.count
            error('wrong data size');
        end
        s.record.custdata = [s.record.custdata(:, (s.recv.count + 1) : end), custdata];
        s.ks.data = [s.getRecordData(); s.getCustomizeData()];
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

    function param = getParameter( s, index, typestr )
        param = double(typecast(uint8(s.ks.info(3:4, index)'), typestr));
    end

	function typeOutput = typeConv( ~, varargin )
        typeInput = varargin{1};
        if isequal(class(typeInput), 'char')
            switch typeInput
                case 'uint8',    typeOutput = 0;
                case 'uint16',   typeOutput = 1;
                case 'uint32',   typeOutput = 2;
                case 'uint64',   typeOutput = 3;
                case 'int8',     typeOutput = 4;
                case 'int16',    typeOutput = 5;
                case 'int32',    typeOutput = 6;
                case 'int64',    typeOutput = 7;
                case 'half',     typeOutput = 9;
                case 'single',   typeOutput = 10;
                case 'double',   typeOutput = 11;
                case 'R0',       typeOutput = 8;
                case 'R1',       typeOutput = 12;
                case 'R2',       typeOutput = 13;
                case 'R3',       typeOutput = 14;
                case 'R4',       typeOutput = 15;
            end
        else
            typeInput = mod(typeInput, 16); % typeInput & 0x0F
            if nargin == 3 && strcmp(varargin{2}, 'cast')
                switch typeInput
                    case 0,  typeOutput = 'uint8';   % 0x0, 4'b 0000
                    case 1,  typeOutput = 'uint16';  % 0x1, 4'b 0001
                    case 2,  typeOutput = 'uint32';  % 0x2, 4'b 0010
                    case 3,  typeOutput = 'uint64';  % 0x3, 4'b 0011
                    case 4,  typeOutput = 'int8';    % 0x4, 4'b 0000
                    case 5,  typeOutput = 'int16';   % 0x5, 4'b 0001
                    case 6,  typeOutput = 'int32';   % 0x6, 4'b 0010
                    case 7,  typeOutput = 'int64';   % 0x7, 4'b 0011
                    case 9,  typeOutput = 'half';    % 0x9, 4'b 1001
                    case 10, typeOutput = 'single';  % 0xA, 8'b 1010
                    case 11, typeOutput = 'double';  % 0xB, 8'b 1011
                    case 8,  typeOutput = 'uint8';   % 0x8, 8'b 1000
                    case 12, typeOutput = 'uint8';   % 0xC, 8'b 1100
                    case 13, typeOutput = 'uint8';   % 0xD, 8'b 1101
                    case 14, typeOutput = 'uint8';   % 0xE, 8'b 1110
                    case 15, typeOutput = 'uint8';   % 0xF, 8'b 1111
                end
            else
                switch typeInput
                    case 0,  typeOutput = 'uint8';   % 0x0, 4'b 0000
                    case 1,  typeOutput = 'uint16';  % 0x1, 4'b 0001
                    case 2,  typeOutput = 'uint32';  % 0x2, 4'b 0010
                    case 3,  typeOutput = 'uint64';  % 0x3, 4'b 0011
                    case 4,  typeOutput = 'int8';    % 0x4, 4'b 0000
                    case 5,  typeOutput = 'int16';   % 0x5, 4'b 0001
                    case 6,  typeOutput = 'int32';   % 0x6, 4'b 0010
                    case 7,  typeOutput = 'int64';   % 0x7, 4'b 0011
                    case 9,  typeOutput = 'half';    % 0x9, 4'b 1001
                    case 10, typeOutput = 'single';  % 0xA, 8'b 1010
                    case 11, typeOutput = 'double';  % 0xB, 8'b 1011
                    case 8,  typeOutput = 'R0';      % 0x8, 8'b 1000
                    case 12, typeOutput = 'R1';      % 0xC, 8'b 1100
                    case 13, typeOutput = 'R2';      % 0xD, 8'b 1101
                    case 14, typeOutput = 'R3';      % 0xE, 8'b 1110
                    case 15, typeOutput = 'R4';      % 0xF, 8'b 1111
                end
            end
        end
    end

	function bytes = typeSize( ~, type )
        if isequal(class(type), 'char')
            switch type
                case 'uint8',   bytes = 1;
                case 'uint16',  bytes = 2;
                case 'uint32',  bytes = 4;
                case 'uint64',  bytes = 8;
                case 'int8',    bytes = 1;
                case 'int16',   bytes = 2;
                case 'int32',   bytes = 4;
                case 'int64',   bytes = 8;
                case 'half',    bytes = 2;
                case 'single',  bytes = 4;
                case 'double',  bytes = 8;
                case 'R0',      bytes = 1;
                case 'R1',      bytes = 1;
                case 'R2',      bytes = 1;
                case 'R3',      bytes = 1;
                case 'R4',      bytes = 1;
            end
        else
            switch type
                case 0,  bytes = 1;  % uint8
                case 1,  bytes = 2;  % uint16
                case 2,  bytes = 4;  % uint32
                case 3,  bytes = 8;  % uint64
                case 4,  bytes = 1;  % int8
                case 5,  bytes = 2;  % int16
                case 6,  bytes = 4;  % int32
                case 7,  bytes = 8;  % int64
                case 9,  bytes = 2;  % half
                case 10, bytes = 4;  % single
                case 11, bytes = 8;  % double
                case 8,  bytes = 1;  % R0
                case 12, bytes = 1;  % R1
                case 13, bytes = 1;  % R2
                case 14, bytes = 1;  % R3
                case 15, bytes = 1;  % R4
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
            case 'half',    convdata = typecast(half(data),   'uint8');
            case 'single',  convdata = typecast(single(data), 'uint8');
            case 'double',  convdata = typecast(double(data), 'uint8');
            case 'R0',      convdata = typecast(uint8(data),  'uint8');
            case 'R1',      convdata = typecast(uint8(data),  'uint8');
            case 'R2',      convdata = typecast(uint8(data),  'uint8');
            case 'R3',      convdata = typecast(uint8(data),  'uint8');
            case 'R4',      convdata = typecast(uint8(data),  'uint8');
        end
    end

    function [lens, type, data] = getTypesAndLens( s, data, typestr )
        [b, i] = max(size(data));
        lens   = s.typeSize(typestr) * b;
        type   = s.typeConv(typestr) + fix(lens / 256) * 16;
        lens   = mod(lens, 256);
        if i == 1
            data = data(:, 1)';
        elseif i == 2
            data = data(1, :);
        end
    end

    function checksum = getChecksum( ~, packet )
        checksum = mod(sum(packet', 2), 256);
    end

end

methods (Access = private)

end

end
