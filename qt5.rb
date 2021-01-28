# frozen_string_literal: true

require 'mikutter_qt5_ext'
require_relative 'delayer_hack'
require_relative 'mainloop'

Plugin.create(:qt5) do
  Plugin::Qt5.init(self)

  def enqueue_delayer_run
    Plugin::Qt5.enqueue do
      Delayer.run
    end
  end

  Delayer.register_remain_hook do
    enqueue_delayer_run
  end

  Delayer.register_reserve_hook do |delay|
    Plugin::Qt5.enqueue_delayed(delay) do
      Delayer.run
    end
  end

  enqueue_delayer_run
end
