.pragma library

function pieces(points, radius) {
    var result = []
    var start = points[0]

    for (var i = 1; i < points.length; i++) {
        var p = points[i]

        if (i === points.length - 1) {
            result.push(makeLine(start, p))
            break
        }

        var next = points[i + 1]
        var d1 = unit(p.x - points[i - 1].x, p.y - points[i - 1].y)
        var d2 = unit(next.x - p.x, next.y - p.y)
        var cross = d1.x * d2.y - d1.y * d2.x
        var theta = Math.acos(Math.max(-1, Math.min(1, d1.x * d2.x + d1.y * d2.y)))
        var cut = radius * Math.tan(theta / 2)

        var t1 = Qt.point(p.x - d1.x * cut, p.y - d1.y * cut)
        var t2 = Qt.point(p.x + d2.x * cut, p.y + d2.y * cut)
        var sign = cross > 0 ? 1 : -1
        var center = Qt.point(t1.x - sign * d1.y * radius, t1.y + sign * d1.x * radius)

        result.push(makeLine(start, t1))
        result.push({
            type: "arc", center: center, radius: radius, end: t2, sign: sign,
            from: Math.atan2(t1.y - center.y, t1.x - center.x),
            sweep: sign * theta,
            length: radius * theta
        })
        start = t2
    }
    return result
}

function unit(dx, dy) {
    var len = Math.sqrt(dx * dx + dy * dy)
    return { x: dx / len, y: dy / len }
}

function makeLine(a, b) {
    return { type: "line", a: a, b: b, length: Math.sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y)) }
}

function svgPath(points, radius) {
    if (points.length < 2)
        return ""
    var list = pieces(points, radius)
    var s = "M " + points[0].x + " " + points[0].y
    for (var i = 0; i < list.length; i++) {
        var k = list[i]
        if (k.type === "line")
            s += " L " + k.b.x + " " + k.b.y
        else
            s += " A " + k.radius + " " + k.radius + " 0 0 " + (k.sign > 0 ? 1 : 0) + " " + k.end.x + " " + k.end.y
    }
    return s
}

function marks(points, radius, step) {
    if (points.length < 2)
        return []
    var list = pieces(points, radius)
    var total = 0
    for (var i = 0; i < list.length; i++)
        total += list[i].length

    var count = Math.max(1, Math.floor(total / step))
    var pad = (total - count * step) / 2
    var result = []

    for (var n = 0; n < count; n++) {
        var s = pad + step / 2 + step * n
        var rest = s
        for (var j = 0; j < list.length; j++) {
            var k = list[j]
            if (rest > k.length && j < list.length - 1) {
                rest -= k.length
                continue
            }
            if (k.type === "line") {
                var f = k.length > 0 ? rest / k.length : 0
                result.push({
                    x: k.a.x + (k.b.x - k.a.x) * f,
                    y: k.a.y + (k.b.y - k.a.y) * f,
                    angle: Math.atan2(k.b.y - k.a.y, k.b.x - k.a.x) * 180 / Math.PI,
                    s: s
                })
            } else {
                var a = k.from + k.sweep * (rest / k.length)
                result.push({
                    x: k.center.x + k.radius * Math.cos(a),
                    y: k.center.y + k.radius * Math.sin(a),
                    angle: a * 180 / Math.PI + k.sign * 90,
                    s: s
                })
            }
            break
        }
    }
    return result
}
