classdef ring_buffer < handle
%a string array ring buffer

    properties (SetAccess = private)
        available
        used
        size
        buffer_struct
    end
    
    properties (Hidden = true, SetAccess = private)
        write_index
        read_index
        class_type
        empty_type_object
    end
        
	methods
        function [obj] = ring_buffer(set_size, example_object)
            assert(isscalar(set_size))
            assert(set_size > 0)

            obj.size = set_size;
            obj.available = set_size;
            obj.used = 0;
            obj.read_index = 1;
            obj.write_index = 1;

            obj.buffer_struct = struct([]);
            
            %set the class_type, default to char
            if nargin < 2
                obj.class_type = 'char';
            else
                obj.class_type = class(example_object);
            end
            
            %buffer size is 1 larger than size
            %create empty object at the last buffer location
            obj.empty_type_object = eval(strcat(obj.class_type,'.empty'));
            obj.buffer_struct(set_size+1,1).object = obj.empty_type_object;
        end

        function [read_object] = read(obj)
            
            if (obj.used == 0)
                read_object = obj.empty_type_object;
                return;
            end

            read_object = obj.buffer_struct(obj.read_index).object;
            obj.read_index = obj.read_index + 1;
            if (obj.read_index > obj.size + 1)
                obj.read_index = 1;
            end

            if (obj.read_index <= obj.write_index)
                obj.used = obj.write_index - obj.read_index;
            else
                obj.used = obj.size + 1 - (obj.read_index - obj.write_index);
            end

            obj.available = obj.size - obj.used;
        end
       
        function [] = write(obj, write_object)
            %removing check of type 
            %assert(isa(write_object, obj.class_type), 'wrong type of object')
            
            obj.buffer_struct(obj.write_index).object = write_object;

            obj.write_index = obj.write_index + 1;
            if (obj.write_index > obj.size + 1)
                obj.write_index = 1;
            end

            if (obj.write_index == obj.read_index)
                obj.read_index = obj.read_index + 1;
                if (obj.read_index > obj.size + 1)
                    obj.read_index = 1;
                end
            end

            if (obj.read_index <= obj.write_index)
                obj.used = obj.write_index - obj.read_index;
            else
                obj.used = obj.size + 1 - (obj.read_index - obj.write_index);
            end

            obj.available = obj.size - obj.used;
            
        end
       
        %peak without removing from the stack
        function [peak_object] = peak(obj)
            peak_object = obj.empty_type_object;
            if (obj.used == 0)
                return;
            end
            peak_object = obj.buffer_struct(obj.read_index).object;
        end
       
        function [] = clear(obj)
            obj.read_index = 1;
            obj.write_index = 1;
            obj.buffer_struct = struct([]);
            obj.buffer_struct(obj.size+1,1).object = obj.empty_type_object;
            obj.update_size();
        end
       
    end 

    methods (Hidden = true)
        
        function [] = update_size(obj)
            if (obj.read_index <= obj.write_index)
                obj.used = obj.write_index - obj.read_index;
            else
                obj.used = obj.size + 1 - (obj.read_index - obj.write_index);
            end
            
            obj.available = obj.size - obj.used;
        end
    end
end
