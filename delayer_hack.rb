# frozen_string_literal: true

module Delayer::Extend
  def reserve(procedure)
    lock.synchronize do
      if @last_reserve
        if @last_reserve > procedure
          @reserves.add(@last_reserve)
          @last_reserve = procedure
        else
          @reserves.add(procedure)
        end
      else
        @last_reserve = procedure
      end
    end
    # -- begin patch --
    @reserve_hook&.call([procedure.reserve_at - Process.clock_gettime(Process::CLOCK_MONOTONIC), 0].max)
    # -- end patch --
    self
  end
  
  def register_reserve_hook(&proc)
    @reserve_hook = proc
  end
end
