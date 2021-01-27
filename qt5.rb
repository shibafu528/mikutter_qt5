# frozen_string_literal: true

require 'mikutter_qt5_ext'
require_relative 'delayer_hack'
require_relative 'mainloop'

Plugin.create(:qt5) do
  Plugin::CPlugin.init(self)

  def enqueue_delayer_run
    Plugin::CPlugin.enqueue do
      Delayer.run
    end
  end

  Delayer.register_remain_hook do
    enqueue_delayer_run
  end

  Delayer.register_reserve_hook do |delay|
    Plugin::CPlugin.enqueue_delayed(delay) do
      Delayer.run
    end
  end

  enqueue_delayer_run

  # TODO: けす
  Delayer.new do
    # なんかこの辺でGCで死ぬ
    Plugin[:world].load_world
    pp Plugin.collect(:worlds).to_a
  end
end
