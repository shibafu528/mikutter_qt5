# frozen_string_literal: true

require 'mikutter_qt5_ext'
require_relative 'mainloop'

Plugin.create(:qt5) do
  Plugin::Qt5.init(self)

  def enqueue_delayer_run
    Plugin::Qt5.enqueue do
      Delayer.run
    end
  end

  Delayer.register_remain_hook(&method(:enqueue_delayer_run))

  enqueue_delayer_run

  filter_gui_timeline_select_messages do |i_timeline, messages|
    # TODO: これだと常に挿入されるから、C++側で判定しないとだめ
    [i_timeline, []]
  end
end
