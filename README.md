![Linux Build](https://github.com/werckme/werckmeister-vst/workflows/Linux%20Build/badge.svg)
![Windows Build](https://github.com/werckme/werckmeister-vst/workflows/Windows%20Build/badge.svg)
![Mac Build](https://github.com/werckme/werckmeister-vst/workflows/Mac%20Build/badge.svg)

# werckmeister-vst
The official werckmeister VST Plugin

https://www.youtube.com/watch?v=AiYw37mhTTo

More Info:
https://werckme.github.io/vst

## Plugin can't be opened on a Mac
If you get a message like:
> Werckmeister.vst can't be opened because its from an unidentified developer

You can override this by executing the following command:  
```sudo xattr -rd com.apple.quarantine <<Path to Plugin>>```

for example:

```sudo xattr -rd com.apple.quarantine ~/myVsts/Werckmeister\ VST.vst3```
