# frozen_string_literal: true

source "https://rubygems.org"

git_source(:github) {|repo_name| "https://github.com/#{repo_name}" }

# ----- ローカルgemを強制的にビルドさせるモンキーパッチ -----
class Bundler::Source::BuildPath < Bundler::Source::Path
  def install(spec, options = {})
    print_using_message "[monkey-patch!!] Using #{version_message(spec)} from #{self}"
    generate_bin(spec, :disable_extensions => false)
    nil # no post-install message
  end
end

Bundler::Dsl.prepend Module.new {
  def valid_keys
    super + %w[build]
  end
}

Bundler::SourceList.prepend Module.new {
  def add_path_source(options = {})
    if options["build"]
      add_source_to_list Bundler::Source::BuildPath.new(options), path_sources
    else
      super
    end
  end
}
# ----------------

gem "mikutter_qt5_ext", path: File.expand_path('./mikutter_qt5_ext', File.dirname(__FILE__)), build: true
