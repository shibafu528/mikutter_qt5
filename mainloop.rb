# frozen_string_literal: true

module Mainloop

  def before_mainloop
    # Gtk.init_add{ Gtk.quit_add(Gtk.main_level){ SerialThreadGroup.force_exit! } }
  end

  def mainloop
    # Gtk.main
    Plugin::Qt5.mainloop
  rescue Interrupt, SystemExit, SignalException => exception
    raise exception
  rescue Exception => exception
    # Gtk.exception = exception
    raise exception
  ensure
    SerialThreadGroup.force_exit!
  end

  def exception_filter(e)
    # Gtk.exception ? Gtk.exception : e
    e
  end

end
