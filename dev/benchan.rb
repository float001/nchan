#!/usr/bin/env ruby
require 'rubygems'
require 'bundler/setup'

require 'securerandom'
require_relative 'pubsub.rb'
require "optparse"
require 'timers'
require 'json'
require "pry"
require "HDRHistogram"

verbose = false

opt_parser=OptionParser.new do |opts|
  opts.on("-v", "--verbose", "somewhat rather extraneously wordful output") do
    verbose = true
  end
end
opt_parser.banner="Usage: benchan.rb [options] url1 url2 url3..."
opt_parser.parse!

urls = []
urls += ARGV
begin
  urls += STDIN.read_nonblock(100000).split /\s*\n+\s*/
rescue IO::WaitReadable
end

urls.uniq!

class Benchan
  def initialize(urls)
    @urls = urls
    @n = urls.count
    @initializing = 0
    @ready = 0
    @running = 0
    @finished = 0
    @subs = []
    @results = {}
    @failed = {}
    
    @hdrh_publish = nil
    @hdrh_receive = nil
    
    subs = []
  end
  
  def run
    puts "connecting to #{@n} Nchan server#{@n > 1 ? "s" : ""}..."
    @urls.each do |url|
      sub = Subscriber.new(url, 1, client: :websocket, timeout: 900000, extra_headers: {"Accept" => "text/x-json-hdrhistogram"})
      sub.on_failure do |err|
        unless @results[sub]
          unless @results[sub.url]
            @failed[sub] = true
            abort err, sub
          end
        end
        false
      end
      sub.on_message do |msg|
        msg = msg.to_s
        case msg
        when "READY"
          puts   "  #{sub.url} ok"
          @ready +=1
          if @ready == @n
            control :run
            puts "start benchmark..."
          end
        when "RUNNING"
          puts   "  #{sub.url} running"
        when /^RESULTS\n/
          msg = msg[8..-1]
          parsed = JSON.parse msg
          @results[sub.url] = parsed
          1+1
        else
          binding.pry
          1+1
        end
      end
      @subs << sub
      sub.run
      sub.wait :ready, 1
      if @failed[sub]
        puts "  #{sub.url} failed"
      else
        puts "  #{sub.url} ok"
      end
    end
    return if @failed.count > 0
    puts "initializing benchmark..."
    control :initialize
    self.wait
    puts "finished."
    puts ""
  end
  
  def wait
    @subs.each &:wait
  end
  
  def control(msg)
    @subs.each { |sub| sub.client.send_data msg.to_s }
  end
  
  def abort(err, src_sub = nil)
    puts "  #{err}"
    @subs.each do |sub|
      sub.terminate unless sub == src_sub
    end
  end
  
  def hdrhistogram_stats(name, histogram)
    fmt = <<-END.gsub(/^ {6}/, '')
      %s
        min:                         %.3fms
        avg:                         %.3fms
        99%%ile:                      %.3fms
        max:                         %.3fms
        stddev:                      %.3fms
        samples:                     %d
    END
    fmt % [ name,
      histogram.min, histogram.mean, histogram.percentile(99.0), histogram.max, histogram.stddev, histogram.count
    ]
  end
  
  def results
    channels = 0
    runtime = []
    subscribers = 0
    message_length = []
    messages_sent = 0
    messages_send_failed = 0
    messages_received = 0
    messages_unreceived = 0
    hdrh_publish = nil
    hdrh_receive = nil
    @results.each do |url, data|
      channels += data["channels"]
      runtime << data["run_time_sec"]
      subscribers += data["subscribers"]
      message_length << data["message_length"]
      messages_sent += data["messages"]["sent"]
      messages_send_failed += data["messages"]["send_failed"]
      messages_received += data["messages"]["received"]
      messages_unreceived += data["messages"]["unreceived"]
      
      if data["message_publishing_histogram"]
        hdrh = HDRHistogram.unserialize(data["message_publishing_histogram"], unit: :ms, multiplier: 0.001)
        if hdrh_publish
          hdrh_publish.merge! hdrh
        else
          hdrh_publish = hdrh
        end
      end
      if data["message_delivery_histogram"]
        hdrh = HDRHistogram.unserialize(data["message_delivery_histogram"], unit: :ms, multiplier: 0.001)
        if hdrh_receive
          hdrh_receive.merge! hdrh
        else
          hdrh_receive = hdrh
        end
      end
    end
    
    message_length.uniq!
    runtime.uniq!
    
    fmt = <<-END.gsub(/^ {6}/, '')
      Nchan servers:                 %d
      runtime:                       %s
      channels:                      %d
      subscribers:                   %d
      subscribers per channel:       %.1f
      messages:
        length:                      %s
        sent:                        %d
        send_failed:                 %d
        received:                    %d
        unreceived:                  %d
        send rate:                   %.3f/sec
        receive rate:                %.3f/sec
        send rate per channel:       %.3f/min
        receive rate per subscriber: %.3f/min
    END
    out = fmt % [
      @n, runtime.join(","), channels, subscribers, subscribers.to_f/channels,
      message_length.join(","), messages_sent, messages_send_failed, 
      messages_received, messages_unreceived,
      messages_sent.to_f/runtime.max,
      messages_received.to_f/runtime.max,
      (messages_sent.to_f* 60)/(runtime.max*channels),
      (messages_received.to_f * 60)/(runtime.max * subscribers)
    ]
    
    out << hdrhistogram_stats("message publishing latency", hdrh_publish) if hdrh_publish
    out << hdrhistogram_stats("message delivery latency", hdrh_receive) if hdrh_receive
    
    puts out
  end
end

benchan = Benchan.new urls
benchan.run
benchan.results
