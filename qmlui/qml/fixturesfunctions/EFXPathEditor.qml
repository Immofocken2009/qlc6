/*
  Q Light Controller Plus
  EFXPathEditor.qml

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

import QtQuick
import QtQuick.Controls

import "."

/**
 * Visual editor for the shape (path) of an EFX effect.
 *
 * Works in the raw -1..1 pan/tilt space of the effect, drawn like the EFX preview
 * (x to the right, y downwards). All changes go to efxEditor, which updates the
 * live preview.
 *
 * Tools:
 *  - Move:  drag anchors and Bezier handles of the selected point
 *  - Add:   click on the path to add a point, away from the path to append one
 *  - Draw:  draw a free stroke, it replaces the shape (simplified to a few points)
 */
Column
{
    id: editorRoot
    width: parent ? parent.width : 300
    spacing: 4

    property var points: efxEditor.pathPoints
    property string tool: "move" // move, add, draw
    property int selected: -1
    property var stroke: []

    // drag state
    property int dragKind: 0 // 0 none, 1 anchor, 2 incoming handle, 3 outgoing handle

    readonly property real handleRadius: 6
    readonly property real anchorSize: 12

    onPointsChanged:
    {
        if (selected >= points.length)
            selected = points.length - 1
        pathCanvas.requestPaint()
    }
    onSelectedChanged: pathCanvas.requestPaint()
    onToolChanged: pathCanvas.requestPaint()

    function toPixelX(v) { return (v + 1) * pathCanvas.width / 2 }
    function toPixelY(v) { return (v + 1) * pathCanvas.height / 2 }
    function fromPixelX(px) { return px * 2 / pathCanvas.width - 1 }
    function fromPixelY(py) { return py * 2 / pathCanvas.height - 1 }

    function dist(px, py, x, y)
    {
        var dx = px - toPixelX(x), dy = py - toPixelY(y)
        return Math.sqrt(dx * dx + dy * dy)
    }

    // ---- toolbar ----
    Flow
    {
        width: parent.width
        spacing: 4

        Repeater
        {
            model: [
                { id: "move", text: qsTr("Move") },
                { id: "add", text: qsTr("Add points") },
                { id: "draw", text: qsTr("Draw freehand") }
            ]

            GenericButton
            {
                width: Math.max(contentWidth + 20, 90)
                height: UISettings.listItemHeight
                label: modelData.text
                bgColor: editorRoot.tool === modelData.id ? UISettings.highlight : UISettings.bgControl
                onClicked: editorRoot.tool = modelData.id
            }
        }

        GenericButton
        {
            property bool isCorner: editorRoot.selected >= 0 && editorRoot.selected < editorRoot.points.length &&
                                    editorRoot.points[editorRoot.selected].corner
            width: Math.max(contentWidth + 20, 90)
            height: UISettings.listItemHeight
            label: isCorner ? qsTr("Make smooth") : qsTr("Make corner")
            enabled: editorRoot.selected >= 0
            opacity: enabled ? 1.0 : 0.4
            onClicked: efxEditor.setPathPointCorner(editorRoot.selected, !isCorner)
        }

        GenericButton
        {
            width: Math.max(contentWidth + 20, 90)
            height: UISettings.listItemHeight
            label: qsTr("Delete point")
            enabled: editorRoot.selected >= 0 && editorRoot.points.length > 2
            opacity: enabled ? 1.0 : 0.4
            onClicked:
            {
                efxEditor.deletePathPoint(editorRoot.selected)
                editorRoot.selected = -1
            }
        }

        GenericButton
        {
            width: Math.max(contentWidth + 20, 90)
            height: UISettings.listItemHeight
            label: efxEditor.pathClosed ? qsTr("Closed") : qsTr("Open")
            bgColor: efxEditor.pathClosed ? UISettings.highlight : UISettings.bgControl
            onClicked: efxEditor.pathClosed = !efxEditor.pathClosed
        }
    }

    // ---- canvas ----
    Rectangle
    {
        width: Math.min(parent.width, 460)
        height: width
        color: "white"
        border.width: 1
        border.color: UISettings.bgLight
        anchors.horizontalCenter: parent.horizontalCenter

        Canvas
        {
            id: pathCanvas
            anchors.fill: parent
            antialiasing: true
            contextType: "2d"

            onPaint:
            {
                var ctx = context
                var pts = editorRoot.points
                ctx.reset()
                ctx.clearRect(0, 0, width, height)
                ctx.fillStyle = "white"
                ctx.fillRect(0, 0, width, height)

                // axes and the -1..1 limit
                ctx.lineWidth = 1
                ctx.strokeStyle = "#cccccc"
                ctx.beginPath()
                ctx.moveTo(width / 2, 0); ctx.lineTo(width / 2, height)
                ctx.moveTo(0, height / 2); ctx.lineTo(width, height / 2)
                ctx.stroke()

                if (pts.length >= 2)
                {
                    // the path
                    ctx.lineWidth = 2
                    ctx.strokeStyle = "#0978FF"
                    ctx.beginPath()
                    ctx.moveTo(toPixelX(pts[0].x), toPixelY(pts[0].y))
                    var segs = efxEditor.pathClosed ? pts.length : pts.length - 1
                    for (var s = 0; s < segs; s++)
                    {
                        var a = pts[s], b = pts[(s + 1) % pts.length]
                        ctx.bezierCurveTo(toPixelX(a.ox), toPixelY(a.oy),
                                          toPixelX(b.ix), toPixelY(b.iy),
                                          toPixelX(b.x), toPixelY(b.y))
                    }
                    ctx.stroke()

                    // handles of the selected point
                    var sel = editorRoot.selected
                    if (sel >= 0 && sel < pts.length && editorRoot.tool !== "draw" && !pts[sel].corner)
                    {
                        var p = pts[sel]
                        ctx.lineWidth = 1
                        ctx.strokeStyle = "#888888"
                        ctx.fillStyle = "#ff9500"
                        ctx.beginPath()
                        ctx.moveTo(toPixelX(p.ix), toPixelY(p.iy)); ctx.lineTo(toPixelX(p.x), toPixelY(p.y))
                        ctx.lineTo(toPixelX(p.ox), toPixelY(p.oy))
                        ctx.stroke()
                        var hs = [[p.ix, p.iy], [p.ox, p.oy]]
                        for (var h = 0; h < 2; h++)
                        {
                            ctx.beginPath()
                            ctx.arc(toPixelX(hs[h][0]), toPixelY(hs[h][1]), editorRoot.handleRadius, 0, 2 * Math.PI)
                            ctx.fill()
                        }
                    }

                    // anchors: the first one is green, it is where the movement starts
                    var size = editorRoot.anchorSize
                    for (var i = 0; i < pts.length; i++)
                    {
                        ctx.fillStyle = i === sel ? "#ff9500" : (i === 0 ? "#2ecc40" : "#222222")
                        ctx.fillRect(toPixelX(pts[i].x) - size / 2, toPixelY(pts[i].y) - size / 2, size, size)
                    }
                }

                // the freehand stroke being drawn
                var st = editorRoot.stroke
                if (st.length >= 4)
                {
                    ctx.lineWidth = 2
                    ctx.strokeStyle = "#e91e63"
                    ctx.beginPath()
                    ctx.moveTo(toPixelX(st[0]), toPixelY(st[1]))
                    for (var k = 2; k + 1 < st.length; k += 2)
                        ctx.lineTo(toPixelX(st[k]), toPixelY(st[k + 1]))
                    ctx.stroke()
                }
            }
        }

        MouseArea
        {
            id: canvasMouse
            anchors.fill: parent
            preventStealing: true

            function clampUnit(v) { return Math.max(-1, Math.min(1, v)) }

            onPressed: (mouse) =>
            {
                var pts = editorRoot.points
                editorRoot.dragKind = 0

                if (editorRoot.tool === "draw")
                {
                    editorRoot.stroke = [clampUnit(fromPixelX(mouse.x)), clampUnit(fromPixelY(mouse.y))]
                    return
                }

                if (editorRoot.tool === "add")
                {
                    var idx = efxEditor.insertPathPoint(fromPixelX(mouse.x), fromPixelY(mouse.y))
                    if (idx >= 0)
                    {
                        editorRoot.selected = idx
                        editorRoot.dragKind = 1
                    }
                    return
                }

                // move tool: handles of the selected point first, then anchors (topmost last)
                var sel = editorRoot.selected
                if (sel >= 0 && sel < pts.length && !pts[sel].corner)
                {
                    if (dist(mouse.x, mouse.y, pts[sel].ox, pts[sel].oy) < editorRoot.handleRadius * 2)
                    {
                        editorRoot.dragKind = 3
                        return
                    }
                    if (dist(mouse.x, mouse.y, pts[sel].ix, pts[sel].iy) < editorRoot.handleRadius * 2)
                    {
                        editorRoot.dragKind = 2
                        return
                    }
                }

                var best = -1, bestD = editorRoot.anchorSize * 1.5
                for (var i = pts.length - 1; i >= 0; i--)
                {
                    var d = dist(mouse.x, mouse.y, pts[i].x, pts[i].y)
                    if (d < bestD)
                    {
                        best = i
                        bestD = d
                    }
                }
                editorRoot.selected = best
                editorRoot.dragKind = best >= 0 ? 1 : 0
            }

            onPositionChanged: (mouse) =>
            {
                var x = fromPixelX(mouse.x), y = fromPixelY(mouse.y)

                if (editorRoot.tool === "draw")
                {
                    if (editorRoot.stroke.length === 0)
                        return
                    var st = editorRoot.stroke
                    var lx = st[st.length - 2], ly = st[st.length - 1]
                    x = clampUnit(x)
                    y = clampUnit(y)
                    // skip samples closer than ~1.5 px
                    if (Math.abs(x - lx) + Math.abs(y - ly) > 0.008)
                    {
                        st.push(x, y)
                        editorRoot.stroke = st
                        pathCanvas.requestPaint()
                    }
                    return
                }

                if (editorRoot.selected < 0)
                    return

                if (editorRoot.dragKind === 1)
                    efxEditor.movePathPoint(editorRoot.selected, clampUnit(x), clampUnit(y))
                else if (editorRoot.dragKind === 2)
                    efxEditor.movePathHandle(editorRoot.selected, false, x, y, true)
                else if (editorRoot.dragKind === 3)
                    efxEditor.movePathHandle(editorRoot.selected, true, x, y, true)
            }

            onReleased:
            {
                if (editorRoot.tool === "draw" && editorRoot.stroke.length >= 4)
                {
                    efxEditor.setPathStroke(editorRoot.stroke)
                    editorRoot.selected = -1
                    editorRoot.tool = "move"
                }
                editorRoot.stroke = []
                editorRoot.dragKind = 0
                pathCanvas.requestPaint()
            }
        }
    }

    RobotoText
    {
        width: parent.width
        wrapMode: Text.WordWrap
        fontSize: UISettings.textSizeDefault * 0.8
        label: editorRoot.tool === "move" ?
                   qsTr("Drag the squares to move points. Select a point to get its curve handles (orange).") :
               editorRoot.tool === "add" ?
                   qsTr("Click on the path to add a point there, click elsewhere to append a corner. Drag to place it.") :
                   qsTr("Draw the new shape with the mouse or your finger. Finishing near the start closes it.")
    }

    // ---- save / cancel ----
    Row
    {
        width: parent.width
        spacing: 4

        TextField
        {
            id: shapeName
            width: parent.width - saveButton.width - cancelButton.width - 8
            height: UISettings.listItemHeight
            placeholderText: qsTr("Name of the new shape")
            text: ""
            color: UISettings.fgMain
            background: Rectangle { color: UISettings.bgMedium; border.color: UISettings.bgLight }
        }

        GenericButton
        {
            id: saveButton
            width: Math.max(contentWidth + 20, 80)
            height: UISettings.listItemHeight
            label: qsTr("Save")
            bgColor: "#2e7d32"
            onClicked:
            {
                var err = efxEditor.commitPathEdit(shapeName.text)
                saveError.label = err
                if (err === "")
                    shapeName.text = ""
            }
        }

        GenericButton
        {
            id: cancelButton
            width: Math.max(contentWidth + 20, 80)
            height: UISettings.listItemHeight
            label: qsTr("Cancel")
            onClicked:
            {
                saveError.label = ""
                efxEditor.cancelPathEdit()
            }
        }
    }

    RobotoText
    {
        id: saveError
        width: parent.width
        wrapMode: Text.WordWrap
        visible: label !== ""
        labelColor: "#ff5252"
    }
}
