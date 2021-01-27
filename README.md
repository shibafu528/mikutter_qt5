mikutter_qt5
====

## これは？
下記を目的としたネタプラグインです。

- GTK以外にも、GUIプラグインと組み合わせて動かせるんだよっていうサンプル
- プラグイン内にC拡張を仕込むと何でもできて楽しい！

分かりますね？本気でalternative実装を作る気はないし、そんな時間はありませんので……

## メモ
mikutter/Gemfile に適当にパッチする必要あり。

```patch
@@ -34,6 +34,6 @@ group :plugin do
     eval File.open(path).read
   }
   Dir.glob(File.join(File.expand_path(ENV['MIKUTTER_CONFROOT'] || '~/.mikutter'), 'plugin/*/Gemfile')){ |path|
-    eval File.open(path).read
+    eval File.open(path).read, binding, path
   }
 end
```
