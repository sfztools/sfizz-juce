---
title: "Home"
lang: "en"
layout: "home"
date_fmt: "%B %d, %Y"
---
<div markdown="1" class="jumbotron p-4 mb-3">

***Archived project, current development has moved [here].***

Sfizz is a JUCE based sample player that aims at supporting a healthy part of the
`.sfz` virtual instrument format.
It's still very experimental so expect many bugs and weird stuff.
Any help in testing is greatly appreciated.
If you can code, that's perfect.
The easiest is to load the project through JUCE's Projucer [(see here)]
and export something that matches your preferred IDE.
There is a CMakeFile but it's quite custom and not very flexible yet.
If you can't or don't want to code, that's fine too, just load your favorite
virtual instrument and tell me what seems wrong :)

Check the [Development Status] page to see which opcodes are currently supported.

[Development Status]: {{ "/status" | relative_url }}
[here]: https://sfztools.github.io/sfizz/
[(see here)]: https://juce.com/discover/stories/projucer-manual

<!--
[![Travis](https://img.shields.io/travis/com/sfztools/sfizz-juce.svg?label=Linux-macOS&style=popout&logo=travis)](https://travis-ci.com/sfztools/sfizz-juce)
[![AppVeyor](https://img.shields.io/appveyor/ci/sfztools/sfizz-juce.svg?label=Windows&style=popout&logo=appveyor)](https://ci.appveyor.com/project/sfztools/sfizz-juce)
-->

![Screenshot](/assets/img/screenshot.png)

</div>

<!--
## Latest News

{% include post.html %}
-->
