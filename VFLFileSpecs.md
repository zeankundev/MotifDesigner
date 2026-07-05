# MotifDesigner Visual Files 

MotifDesigner Visual Files (`.vfl`) are core files to build your user interfaces using MotifDesigner. It is the one that MotifDesigner uses to exchange layout or widget info.

Typically, MotifDesigner visual files are meant to be used within MotifDesigner and some other (secretive) tools later on. Usually, they will end with `.vfl`.

## Basic language syntaxing

The syntaxing of MotifDesigner visual files are pretty simple. It's more of tag-based (like XML or HTML) and contains everything about your visuals.

Tags are represented by a pair of brackets. For example, **`[DEFTYPE]` is an opening type, and `[EndDEFTYPE]` is a closing type.** Simple, right?

**`DEFTYPE`** here is substituted with a variety of types in the language. For example, to define a Frame, you will have to type `[Frame]` and `[EndFrame]` to close the tag.

Parameters are defined inside their tags on a separate line. The separator will always be `=`. A line like **`name=value`** is a valid parameter. However, for strings that contain spaces, you do not have to worry, as no quote/speech marks are required to define the outcome. For example:

```
[SampleTag]
param=Hello World
[EndSampleTag]
```

`&&` are used for comments. They must occupy one line. Comments like this are valid:

```
&& Comments!
```

or

```
&& Comment 1
&& Comment 2
```

However, they are not allowed to be inline with a tag or any operand parameters.

```
&& This should not be copied, please :)

[Button]
value=Hello && This is not valid!

&& Above is not valid. MotifDesigner will treat it as part of the value, so "Hello && This is not valid!" will be your value

[EndButton]
```

```
[SampleTag] && This is illegal! This will certainly throw a syntax error!
[EndSampleTag]
```

If you still don't understand, here's an example
```
&& Hello World!

[SampleTag]

[NestedSampleTag]
param=value
[EndNestedSampleTag]

[EndSampleTag]
```
At this language, indenting is perfectly safe, so you can make your visual file clean while also maintaining parseability. You can use either tabs or spaces, depending on your preference.
```
&& Hello World!

[SampleTag]

    [NestedSampleTag]
        param=value
    [EndNestedSampleTag]

[EndSampleTag]
```

## List of Tags and Parameters

A list of tags and parameters will be provided here.
- `[MotifDesignerVisualFile]`: **This is the root tag and is required**. If you do not supply this tag at any point, MotifDesigner cannot parse your visual file.
    - A `Version` parameter is required. This defines the version what version of MotifDesigner Visual File can be opened. These will be **parsed as floats**, which means versions such as `1.0.0` **are not allowed**. However, versions such as `1.0`, `0.73` **are allowed**.

- `[Widgets]`: **This is where you will put all of your widgets**. You can **only** put widgets that you will define here. If you want to leave it empty for the time being, that is **perfectly fine**. No parameters are required.

### Widget Lists (Tags and Generic Parameters)
All widgets defined here must have these parameters, regardless of what they are:
- `x`: x-coordinate of the specified widget
- `y`: y-coordinate of the specified widget
- `width`: width of the specified widget
- `height`: height of the specified widget
- `name`: name of the specified widget. **No foreign characters allowed, and it must be unique. Only A-Z, a-z, 0-9, and underscore `_` are allowed, and it must start with a letter.**
- `value`: value of the widget. No quotes are needed.

Here's an example on what you must use everytime if you want to define a widget. Substitute `[WIDGETNAME]` with your desired widget.

```
[WIDGETNAME]
x=0
y=0
width=0
height=0
name=ExampleName
value=Example Value
[EndWIDGETNAME]
```

Now onto the valid lists of widgets. 
- `[Button]`: Button
- `[Label]`: Label/text
- `[TextField]`: Text field/input
- `[Toggle]`: Toggle/checkbox
- `[Frame]`: Frame/container

## Example of a valid visual file (as a whole)

```
&& MotifDesigner Visual File 
[MotifDesignerVisualFile]
    Version=0.73
    [Widgets]
        [Button]
            x=10
            y=10
            width=120
            height=35
            name=Widget_PushButton1
            value=Click me!
        [EndButton]
        [Label]
            x=10
            y=50
            width=120
            height=20
            name=Widget_Label2
            value=meow :3
        [EndLabel]
    [EndWidgets]
[EndMotifDesignerVisualFile]
```