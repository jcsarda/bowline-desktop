# o = Watcher.new
# o.append('greet') {
#  puts 'hi'
# }
# o.call('greet')
#
# def greet(who)
#  puts "Hi #{who}"
# end
# event = o.append('greet', method(:greet), 'Alex')
# o.call('greet')
# event.remove

module Bowline
  class Watcher
    module Base
      def self.extended(base)
      	base.send :include, InstanceMethods
      end
    
      def watch(*names)
      	names.each do |name|
          # Because define_method only takes a block,
          # which doesn't accept multiple arguments
          script = <<-RUBY
            def on_#{name}(*args, &block)
              watcher.append(:#{name}, *args, &block)
            end
          RUBY
          instance_eval script
          class_eval    script
  			end
      end
      
      def watcher
        @@watcher ||= Watcher.new
      end
    
      module InstanceMethods
        def watcher
        	@watcher ||= Watcher.new
        end
      end
    end
  
    class Callback
      attr_reader :event, :prok
    
      def initialize(watcher, event, prok)
      	@watcher, @event, @prok = watcher, event, prok
      end
    
      def call(*args)
      	@prok.call(*args)
      end
    
      def remove
      	@watcher.remove(@event, @prok)
      end
    end
  
    def initialize
      @listeners = {}
    end
  
    def append(event, method = nil, &block)
      callback = Callback.new(self, event, method||block)
      (@listeners[event] ||= []) << callback
      callback
    end
  
    def call(event, *args)
      return unless @listeners[event]
      @listeners[event].each do |callback|
        callback.call(*args)
      end
    end
  
    def remove(event, value=nil)
      return unless @listeners[event]
      if value
        @listeners[event].delete(value)
        if @listeners[event].empty?
          @listeners.delete(event)
        end
      else
        @listeners.delete(event)
      end
    end
  
    def clear
      @listeners = {}
    end
  end
end