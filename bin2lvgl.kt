/*
   MIT License

  Copyright (c) 2024 Felix Biego
  Modified (c) 2024 Daniel Kampert

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

  ______________  _____
  ___  __/___  /_ ___(_)_____ _______ _______
  __  /_  __  __ \__  / _  _ \__  __ `/_  __ \
  _  __/  _  /_/ /_  /  /  __/_  /_/ / / /_/ /
  /_/     /_.___/ /_/   \___/ _\__, /  \____/
                              /____/

*/

/* TODO
- Environment (Pressure, Humidity, IAQ) is not supported
*/

import java.awt.AlphaComposite
import java.awt.Color
import java.awt.Graphics2D
import java.awt.RenderingHints
import java.awt.image.BufferedImage
import java.awt.image.DataBufferInt
import java.io.File
import java.util.Calendar
import java.util.Random
import javax.imageio.ImageIO

fun byteArrayOfInts(vararg ints: Int) = ByteArray(ints.size) { pos -> ints[pos].toByte() }

fun Byte.toPInt() = toInt() and 0xFF

fun hex(b: Byte): String {
    return b.toPInt().toString(16).padStart(2, '0')
}

fun hex(i: Int): String {
    return i.toString(16).padStart(2, '0')
}

class Item(
        var known: Boolean,
        var name: String
)

class Resource(var id: Int, var pos: Int)

class Show(var id: Int, var value: Int) {
    var offset = 0
    fun getOff(max: Int): Int {
        var dig = getPlaceValue(value, offset)
        if (dig >= max) {
            dig = 0
        }
        offset++
        return dig
    }

    fun getPlaceValue(number: Int, position: Int): Int {
        val numberAsString = number.toString()
        // Check if the position is valid
        if (position < 0 || position >= numberAsString.length) {
            return 0
        }
        // Get the character at the specified position and convert it to an integer
        val digitChar = numberAsString[numberAsString.length - 1 - position]
        return digitChar.toString().toInt()
    }

    fun getLv(max: Int): String {
        val x = (Math.pow(10.0, (offset - 1.0))).toInt()
        return when (id) {
            0x00 -> "(hour / $x) % $max"
            0x01 -> "(minute / $x) % $max"
            0x02 -> "(day / $x) % $max"
            0x03 -> "(month / $x) % $max"
            0x06 -> "((weekday + 6) / $x) % $max"
            0x07 -> "(year / $x) % $max"
            0x08 -> "(am ? 0 : 1) % $max"
            0x0B -> "(battery / $x) % $max"
            0x1B -> "(seconds / $x) % $max"
            0x16 -> "(temp / $x) % $max"
            0x17 -> "icon % 8"
            0x10 -> "(bpm / $x) % $max"
            0x11 -> "(oxygen / $x) % $max"
            0x0E -> "(steps / $x) % $max"
            0x0F -> "(kcal / $x) % $max"
            0x14 -> "(distance / $x) % $max"
            else -> ""
        }
    }

    fun unit(): String {
        return when (id) {
            0x00 -> "hour"
            0x01 -> "minute"
            0x02 -> "day"
            0x03 -> "month"
            0x06 -> "weekday"
            0x07 -> "year"
            0x0B -> "battery"
            0x1B -> "seconds"
            0x16 -> "temp"
            0x17 -> "icon"
            0x10 -> "bpm"
            0x11 -> "oxygen"
            0x0E -> "steps"
            0x0F -> "kcal"
            0x14 -> "distance"
            else -> ""
        }
    }
}

fun group(id: Int): Int {
    return when (id) {
        0x00, 0x01, 0x02, 0x03, 0x06, 0x07, 0x08 -> 1 // time
        0x0A, 0x0B -> 2 // status
        0x0E, 0x0F, 0x14, 0x15 -> 3 // activity
        0x10, 0x11 -> 4 // health
        0x26, 0x16, 0x17, 0xFA -> 5 // weather
        else -> -1
    }
}

fun type(id: Int): Item {
    return when (id) {
        0x00 -> Item(true, "Hour no")
        0x01 -> Item(true, "Minute no")
        0x02 -> Item(true, "Date no")
        0x03 -> Item(true, "Month no")
        0x06 -> Item(true, "Weekday label")
        0x07 -> Item(true, "Year no -")
        0x08 -> Item(true, "AM/PM label")
        0x09 -> Item(true, "Image/Icon")
        0x0A -> Item(true, "Connection label")
        0x0B -> Item(true, "Battery no")
        0x0C -> Item(true, "Sleep label")
        0x0D -> Item(true, "Analog hands")
        0x0E -> Item(true, "Steps no")
        0x0F -> Item(true, "Calories no")
        0x10 -> Item(true, "Heart Rate no")
        0x11 -> Item(true, "SP02 no")
        0x13 -> Item(true, "Sleep no")
        0x14 -> Item(true, "Distance no")
        0x15 -> Item(true, "Distance Label")
        0x16 -> Item(true, "Weather no")
        0x17 -> Item(true, "Weather label")
        0x19 -> Item(true, "Solid color")
        0x1A -> Item(false, "Analog x5")
        0x1B -> Item(true, "Seconds")
        0x1D -> Item(true, "Unknown")
        0x1E -> Item(false, "Analog x8")
        0xFA -> Item(true, "Weather no+ label")
        0xFB -> Item(false, "Click")
        0xFD -> Item(true, "Animation")
        else -> Item(false, "0x" + hex(id) + " -> \"Unknown\"")
    }
}

private val itemShow = ArrayList<Show>()

private fun generateList() {
    val cal = Calendar.getInstance()
    itemShow.clear()
    itemShow.add(Show(0x00, cal.get(Calendar.HOUR_OF_DAY))) // hour
    itemShow.add(Show(0x01, cal.get(Calendar.MINUTE))) // minute
    itemShow.add(Show(0x1B, cal.get(Calendar.SECOND))) // second
    itemShow.add(Show(0x02, cal.get(Calendar.DAY_OF_MONTH))) // date
    itemShow.add(Show(0x03, cal.get(Calendar.MONTH) + 1)) // month
    val day = cal.get(Calendar.DAY_OF_WEEK) - 1
    itemShow.add(Show(0x06, if (day == 0) 6 else day - 1)) // weekday sun - mon -> mon - sun
    itemShow.add(Show(0x07, cal.get(Calendar.YEAR))) // year

    itemShow.add(Show(0x10, 72)) // bpm
    itemShow.add(Show(0x11, 98)) // sp02
    itemShow.add(Show(0x0E, 2735)) // steps
    itemShow.add(Show(0x0F, 163)) // kcal
    itemShow.add(Show(0x0B, 85)) // battery
    itemShow.add(Show(0x14, 157)) // distance
    itemShow.add(Show(0x16, 2222)) // weather temp

    itemShow.add(Show(0x13, 3008)) // sleep
}

fun main(args: Array<String>) {

    if (args.size > 0) {
        val data = File(args[0]).readBytes()

        if (data.size > 0) {
            val nm = args[0].replace(".bin", "").replace("-", "_")
            extractComponents(data, nm)

            println("-----Done-------")
        } else {
            println("Could not read from ${args[0]}")
        }
    } else {
        println("Specify the file name")
    }
}

fun extractComponents(data: ByteArray, name: String, wd: Int = 240, ht: Int = 240) {
    val no =
            (data[3].toPInt() * 256 * 256 * 256) +
                    (data[2].toPInt() * 256 * 256) +
                    (data[1].toPInt() * 256) +
                    data[0].toPInt()

    println("Detected $no components")

    if (no > 100) {
        println("Watchface not valid, exiting")
        return
    }

    generateList()

    val canvas = BufferedImage(wd, ht, BufferedImage.TYPE_INT_ARGB)

    val graphics = canvas.createGraphics()
    graphics.color = Color.BLACK
    graphics.fillRect(0, 0, wd, ht)

    val rsc = arrayListOf<Resource>()
    var lan = 0
    var text = "Components List\n"
    var a = 0
    var wt = 0
    var tp = 0
    var objects = ""
    var declare = ""
    var faceItems = ""
    var rscArray = ""
    var lvUpdateTime = ""
    var lvUpdateWeather = ""
    var lvUpdateBattery = ""
    var lvUpdateNotifications = ""
    var lvUpdateEnvironment = ""
    var lvUpdateConnection = ""
    var lvUpdateActivity = ""
    var lvUpdateHealth = ""
    var weatherIc = "const lv_img_dsc_t *face_${name}_dial_img_weather[] = {\n"
    var connIC = "const lv_img_dsc_t *face_${name}_dial_img_connection[] = {\n"

    // loop through the components
    myLoop@ for (x in 0 until no) {
        val hexId =
                hex(data[(x * 20) + 4]) +
                        hex(data[(x * 20) + 5]) +
                        hex(data[(x * 20) + 6]) +
                        hex(data[(x * 20) + 7])

        val id = data[(x * 20) + 4].toPInt()
        var xOff = (data[(x * 20) + 9].toPInt() * 256) + data[(x * 20) + 8].toPInt()
        var yOff = (data[(x * 20) + 11].toPInt() * 256) + data[(x * 20) + 10].toPInt()
        val xSz = (data[(x * 20) + 13].toPInt() * 256) + data[(x * 20) + 12].toPInt()
        val ySz = (data[(x * 20) + 15].toPInt() * 256) + data[(x * 20) + 14].toPInt()

        val clt =
                (data[(x * 20) + 19].toPInt() * 256 * 256 * 256) +
                        (data[(x * 20) + 18].toPInt() * 256 * 256) +
                        (data[(x * 20) + 17].toPInt() * 256) +
                        data[(x * 20) + 16].toPInt()

        val dat =
                (data[(x * 20) + 23].toPInt() * 256 * 256 * 256) +
                        (data[(x * 20) + 22].toPInt() * 256 * 256) +
                        (data[(x * 20) + 21].toPInt() * 256) +
                        data[(x * 20) + 20].toPInt()

        val id2 = data[(x * 20) + 5].toPInt()

        var isG =
                (data[(x * 20) + 5].toPInt() and 0x80) ==
                        0x80 // check if is grouped in single resource
        if (id == 0x08){
            isG = true
        }
        val cmp = if (isG) data[(x * 20) + 5].toPInt() and 0x7F else 1

        val aOff = data[(x * 20) + 6].toPInt()

        val isM = (data[(x * 20) + 7].toPInt() and 0x80) == 0x80
        val cG = if (isM) data[(x * 20) + 7].toPInt() and 0x7F else 0
        if (isM) {
            lan++
        }

        if (tp == 0x09 && id == 0x09) {
            a++
        } else if (tp != id) {
            tp = id
            a++
        } else if (lan == 1) {
            a++
        }

        text += "$x\t$a\t$lan\t$hexId\t$xOff\t$yOff\t$xSz\t$ySz\t$clt\t$dat\t${type(id).name}\n"

        if (!type(id).known) {
            println("Skipping: $hexId ${type(id).name}")
            continue
        }

        var output = byteArrayOfInts()

        try {
            for (z in 0 until (xSz * ySz)) {
                if (id == 0x19 || (id == 0x16 && (id2 == 0x00 || id2 == 0x06))) {
                    output += data[(x * 20) + 16]
                    output += data[(x * 20) + 17]
                } else if (clt == dat) {
                    output += data[(z * 2) + dat]
                    output += data[(z * 2) + dat + 1]
                } else {
                    output += data[clt + (data[z + dat].toPInt() * 2)]
                    output += data[clt + (data[z + dat].toPInt() * 2) + 1]
                }
            }
        } catch (error: Exception) {
            println("error at $x -> $error")
            continue
        }

        if (xSz > 0 && ySz > 0) {
            var rs = rsc.singleOrNull { it.id == clt }

            var z =
                    if (rs != null) {
                        rs.pos
                    } else {
                        x
                    }

            val drawable =
                    if (id == 0x0d) {
                        (lan == 1 || lan == 17 || lan == 33) // allow only specific analog hands
                    } else {
                        true // otherwise all items are drawable
                    }

            if (rs == null && drawable) {
                rsc.add(Resource(clt, x))

                var rscArr = "const void *face_${name}_dial_img_${x}_${clt}_group[] = {\n"

                // save assets & declare
                for (aa in 0 until cmp) {
                    declare += "ZSW_LV_IMG_DECLARE(face_${name}_dial_img_${x}_${clt}_${aa});\n"
                    rscArr += "    ZSW_LV_IMG_USE(face_${name}_dial_img_${x}_${clt}_${aa}),\n"
                }
                rscArr += "};\n"

                if (id == 0x17) {
                    weatherIc += "    &face_${name}_dial_img_${x}_${clt}_0,\n"
                } else if (id == 0x0A) {
                    connIC += "    &face_${name}_dial_img_${x}_${clt}_0,\n"

                    if (connIC.count { it == '\n' } > 2) {

                        rscArray += connIC + "};\n"
                    }
                } else if (cmp == 1) {
                    // do not add non grouped items
                } else {
                    // do not add weather to group
                    rscArray += rscArr
                }

                saveAsset(
                        output,
                        xSz,
                        ySz / cmp,
                        !(x == 0 && (id == 0x09 || id == 0x19)),
                        cmp,
                        name,
                        "${x}_${clt}"
                )

                if (id == 0x1E) {} else {

                    // saveImage(output, xSz, ySz, x, name, clt)
                }
            } else if (id == 0x16 && id2 == 0x00) {
                saveAsset(
                        output,
                        xSz,
                        ySz / cmp,
                        !(x == 0 && (id == 0x09 || id == 0x19)),
                        cmp,
                        name,
                        "${x}_${clt}"
                )
            }

            if (id == 0x0A) {
                if (connIC.count { it == '\n' } < 3) {
                    continue
                }
            }

            if (isM) {
                if (lan == cG) {
                    lan = 0
                } else if (id == 0x0d &&
                                (lan == 1 || lan == 32 || lan == 40 || lan == 17 || lan == 33)
                ) {
                    yOff -= (ySz - aOff)
                    xOff -= aOff
                } else {
                    continue
                }
            }

            if (id == 0x17) {
                wt++
                if (wt == 9) {
                    rscArray += weatherIc + "};\n"
                }
                if (wt != 1) {
                    continue
                }
            }

            if (id == 0x16 && id2 == 0x06) {
                continue
            }

            if (drawable) {
                objects += "static lv_obj_t *face_${name}_${x}_${clt};\n"

                faceItems +=
                        lvItem.replace("{{PARENT}}", "face_${name}")
                                .replace("{{CHILD}}", "face_${name}_${x}_${clt}")
                                .replace("{{CHILD_X}}", "$xOff")
                                .replace("{{CHILD_Y}}", "$yOff")
                                .replace("{{RESOURCE}}", "face_${name}_dial_img_${z}_${clt}_0")

                if (id == 0x0d) {
                    faceItems +=
                            "    lv_img_set_pivot(face_${name}_${x}_${clt}, $aOff, ${ySz - aOff});\n"
                    if (lan == 1) {
                        // hour hand
                        lvUpdateTime +=
                                "    lv_img_set_angle(face_${name}_${x}_${clt}, hour * 300 + (minute * 5));\n"
                    }
                    if (lan == 17) {
                        // minute hand
                        lvUpdateTime +=
                                "    lv_img_set_angle(face_${name}_${x}_${clt}, minute * 60);\n"
                    }
                    if (lan == 33) {
                        // second hand
                        lvUpdateTime +=
                                "    lv_img_set_angle(face_${name}_${x}_${clt}, second * 60);\n"
                    }
                }
            }
            if (id == 0x16 && id2 == 0x00) {
                lvUpdateWeather +=
                        "    if (temp >= 0)\n    {\n        lv_obj_add_flag(face_${name}_${x}_${clt}, LV_OBJ_FLAG_HIDDEN);\n    } else {\n        lv_obj_clear_flag(face_${name}_${x}_${clt}, LV_OBJ_FLAG_HIDDEN);\n    }\n"
                continue
            }
            if (id == 0x16 && id2 == 0x01) {
                continue
            }

            val sh = itemShow.singleOrNull { it.id == id }
            val offs = sh?.getOff(cmp) ?: Random().nextInt(cmp)
            val lvT = sh?.getLv(cmp) ?: ""
            val unit = sh?.unit() ?: ""
            val place = sh?.offset?:1
            if (lvT.isNotEmpty() && (unit.isNotEmpty()) && !(id == 0x0b && aOff == 0)) {
                when (group(id)) {
                    1 -> {
                            lvUpdateTime +=
"""
    if (getPlaceValue(last_${unit}, $place) != getPlaceValue(${unit}, $place)) {
        last_${unit} = setPlaceValue(last_${unit}, $place, getPlaceValue(${unit}, $place));
        lv_img_set_src(face_${name}_${x}_${clt}, face_${name}_dial_img_${z}_${clt}_group[${lvT}]);
    }
"""

                    }
                    2 -> {
                        lvUpdateBattery +=
                                "    lv_img_set_src(face_${name}_${x}_${clt}, face_${name}_dial_img_${z}_${clt}_group[${lvT}]);\n"
                        if (lvT == "(battery / 100) % 10") {
                            lvUpdateBattery +=
                                    "    if (battery < 100)\n    {\n        lv_obj_add_flag(face_${name}_${x}_${clt}, LV_OBJ_FLAG_HIDDEN);\n    } else {\n        lv_obj_clear_flag(face_${name}_${x}_${clt}, LV_OBJ_FLAG_HIDDEN);\n    }\n"

                                    // do not draw it on the preview
                                    continue@myLoop
                        }
                    }
                    3 -> {
                        lvUpdateActivity +=
"""
    if (getPlaceValue(last_${unit}, $place) != getPlaceValue(${unit}, $place)) {
        last_${unit} = setPlaceValue(last_${unit}, $place, getPlaceValue(${unit}, $place));
        lv_img_set_src(face_${name}_${x}_${clt}, face_${name}_dial_img_${z}_${clt}_group[${lvT}]);
    }
"""
                    }
                    4 -> {
                        lvUpdateHealth +=
                                "    lv_img_set_src(face_${name}_${x}_${clt}, face_${name}_dial_img_${z}_${clt}_group[${lvT}]);\n"
                    }
                    5 -> {
                        lvUpdateWeather +=
                                "    lv_img_set_src(face_${name}_${x}_${clt}, face_${name}_dial_img_${z}_${clt}_group[${lvT}]);\n"
                    }
                }
            }
            if (id == 0x17) {
                lvUpdateWeather +=
                        "    lv_img_set_src(face_${name}_${x}_${clt}, face_${name}_dial_img_weather[icon % 8]);\n"
            }
            if (id == 0x0b && aOff == 0) {
                lvUpdateBattery +=
                        "    lv_img_set_src(face_${name}_${x}_${clt}, face_${name}_dial_img_${z}_${clt}_group[(battery / (100 / ${cmp})) % ${cmp}]);\n"
            }
            if (id == 0x0a) {
                lvUpdateConnection +=
                        "    lv_img_set_src(face_${name}_${x}_${clt}, face_${name}_dial_img_connection[(connection ? 0 : 1) % 2]);\n"
            }
            if (id == 0x08) {
                lvUpdateTime +=
                        "    if (mode)\n    {\n        lv_obj_add_flag(face_${name}_${x}_${clt}, LV_OBJ_FLAG_HIDDEN);\n    } else {\n        lv_obj_clear_flag(face_${name}_${x}_${clt}, LV_OBJ_FLAG_HIDDEN);\n    }\n"
                lvUpdateTime +=
                        "    lv_img_set_src(face_${name}_${x}_${clt}, face_${name}_dial_img_${z}_${clt}_group[(am ? 0 : 1) % 2]);\n"
            }

            if (id == 0x0d && (lan == 17 || lan == 33)) {
                continue // do not draw the analog() on the preview
            }
            if (xSz > 500 || ySz > 5000) {
                // not valid ?
            } else {
                val image = getImage(output, xSz, ySz / cmp, offset = offs)
                graphics.drawImage(image, xOff, yOff, null)
            }
        }
    }

    graphics.dispose()

    // Create a file to save the image to
    val dir = File(name)

    if (!dir.exists()) {
        dir.mkdirs()
        println("Created output folder")
    }

    val list = File(dir, "items.txt")
    list.writeText(text)
    val outputFile = File(dir, "watchface.png")

    // Save the BufferedImage to the output file
    ImageIO.write(canvas, "png", outputFile)

    val scaledCanvas = BufferedImage(160, 160, BufferedImage.TYPE_INT_ARGB)
    val g2d: Graphics2D = scaledCanvas.createGraphics()
    g2d.setRenderingHint(
            RenderingHints.KEY_INTERPOLATION,
            RenderingHints.VALUE_INTERPOLATION_BILINEAR
    )
    g2d.setRenderingHint(RenderingHints.KEY_RENDERING, RenderingHints.VALUE_RENDER_QUALITY)
    g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON)
    g2d.drawImage(canvas, 0, 0, 160, 160, null)
    val mask = BufferedImage(160, 160, BufferedImage.TYPE_INT_ARGB)
    val maskGraphics = mask.createGraphics()
    maskGraphics.color = Color.BLACK
    maskGraphics.fillOval(0, 0, 160, 160)
    maskGraphics.dispose()
    val composite = AlphaComposite.DstIn
    g2d.composite = composite
    g2d.drawImage(mask, 0, 0, null)
    g2d.dispose()

    saveAsset(bufferBytes(scaledCanvas), 160, 160, false, 1, name, "preview")

    declare += "ZSW_LV_IMG_DECLARE(face_${name}_dial_img_preview_0);\n"

    c_file =
            c_file.replace("{{NAME}}", name.toLowerCase())
                    .replace("{{DECLARE}}", declare)
                    .replace("{{OBJECTS}}", objects)
                    .replace("{{ITEMS}}", faceItems)
                    .replace("{{RSC_ARR}}", rscArray)
                    .replace("{{TIME}}", lvUpdateTime)
                    .replace("{{BATTERY}}", lvUpdateBattery)
                    .replace("{{NOTIFICATIONS}}", lvUpdateNotifications)
                    .replace("{{ENVIRONMENT}}", lvUpdateEnvironment)
                    .replace("{{CONNECTION}}", lvUpdateConnection)
                    .replace("{{WEATHER}}", lvUpdateWeather)
                    .replace("{{ACTIVITY}}", lvUpdateActivity)
                    .replace("{{HEALTH}}", lvUpdateHealth)

    val source = File(dir, "zsw_watchface_${name}_ui.c")
    source.writeText(c_file)

    val cmake = File(dir, "CMakeLists.txt")
    cmake_file = cmake_file.replace("{{NAME}}", name.toLowerCase())
    cmake.writeText(cmake_file)
}

fun getImage(
        rgb565: ByteArray,
        width: Int,
        height: Int,
        tr: Boolean = true,
        offset: Int = 0
): BufferedImage {
    // Convert the RGB565 byte array to a BufferedImage with alpha channel
    val image = BufferedImage(width, height, BufferedImage.TYPE_INT_ARGB)
    val raster = image.raster
    val dataBuffer = raster.dataBuffer as DataBufferInt
    val pixels = dataBuffer.data

    for (y in 0 until height) {
        for (x in 0 until width) {
            val i = y * width + x
            val j = y * width + x + (height * width * offset)
            val r = (rgb565[j * 2 + 1].toInt() and 0xF8)
            val g =
                    ((rgb565[j * 2 + 1].toInt() and 0x07) shl 5) or
                            ((rgb565[j * 2].toInt() and 0xE0) shr 3)
            val b = (rgb565[j * 2].toInt() and 0x1F) shl 3

            // Set the alpha channel to 0 for black color
            val alpha = if (r == 0 && g == 0 && b == 0 && tr) 0 else 255

            // Pack the ARGB values into a single int
            val argb = (alpha shl 24) or (r shl 16) or (g shl 8) or b

            pixels[i] = argb
        }
    }

    // return BufferedImage
    return image
}

fun saveImage(rgb565: ByteArray, width: Int, height: Int, no: Int, folder: String, id: Int) {
    // Convert the RGB565 byte array to a BufferedImage
    val image = BufferedImage(width, height, BufferedImage.TYPE_USHORT_565_RGB)
    val dataBuffer = image.raster.dataBuffer as java.awt.image.DataBufferUShort
    for (y in 0 until height) {
        for (x in 0 until width) {
            val i = y * width + x
            val px = (rgb565[i * 2 + 1].toPInt() * 256) + rgb565[i * 2].toPInt()
            dataBuffer.setElem(i, px)
        }
    }

    // Write the BufferedImage to a JPG file
    val dir = File(folder)

    if (!dir.exists()) {
        dir.mkdirs()
        println("Created output folder")
    }

    val outputFile = File(dir, "$no-$id.jpg")
    ImageIO.write(image, "jpg", outputFile)
}

fun bufferBytes(canvas: BufferedImage): ByteArray {
    val width = canvas.width
    val height = canvas.height
    val pixels = IntArray(width * height)
    canvas.getRGB(0, 0, width, height, pixels, 0, width)

    var byteArray = byteArrayOfInts()

    for (pixel in pixels) {
        val red = (pixel shr 19) and 0x1F // Extract red component (5 bits)
        val green = (pixel shr 10) and 0x3F // Extract green component (6 bits)
        val blue = (pixel shr 3) and 0x1F // Extract blue component (5 bits)
        val alpha = (pixel shr 24) and 0xFF // Extract alpha component (8 bits)

        // Convert to RGB565 format with alpha
        val rgb565WithAlpha = (alpha shl 16) or (red shl 11) or (green shl 5) or blue

        // Swap the bytes (Little Endian format)
        byteArray += (rgb565WithAlpha and 0xFF).toByte()
        byteArray += ((rgb565WithAlpha shr 8) and 0xFF).toByte()
    }

    return byteArray
}

fun saveAsset(
        rgb565: ByteArray,
        width: Int,
        height: Int,
        tr: Boolean = true,
        amount: Int,
        name: String,
        asset: String
) {

    for (a in 0 until amount) {
        var dat =
                """
const LV_ATTRIBUTE_MEM_ALIGN uint8_t face_${name}_dial_img_${asset}_data_${a}[] = {
{{BYTES}}
    };
"""
        var text = asset_header.replace("{{NAME}}", name.toLowerCase())

        var bts = "\t"
        var z = 0
        for (y in 0 until height) {
            for (x in 0 until width) {
                // val i = y * width + x
                val j = y * width + x + (height * width * a)
                val r = (rgb565[j * 2 + 1].toInt() and 0xF8)
                val g =
                        ((rgb565[j * 2 + 1].toInt() and 0x07) shl 5) or
                                ((rgb565[j * 2].toInt() and 0xE0) shr 3)
                val b = (rgb565[j * 2].toInt() and 0x1F) shl 3

                // Set the alpha channel to 0 for black color
                val alpha = if (r == 0 && g == 0 && b == 0 && tr) 0 else 255

                var hex =
                        String.format(
                                "0x%02X,0x%02X,",
                                rgb565[j * 2 + 1].toInt() and 0xFF,
                                rgb565[j * 2].toInt() and 0xFF
                        )
                if (tr) {
                    hex += String.format("0x%02X,", alpha and 0xFF)
                }
                if (z % 32 == 0 && z != 0) {
                    hex += "\n\t"
                }
                bts += hex
                z++
            }
        }

        dat = dat.replace("{{BYTES}}", bts)

        text += dat

        val color =
                if (tr) {
                    "LV_IMG_CF_TRUE_COLOR_ALPHA"
                } else {
                    "LV_IMG_CF_TRUE_COLOR"
                }

        val obj =
                """
const lv_img_dsc_t face_${name}_dial_img_${asset}_${a} = {
    .header.always_zero = 0,
    .header.w = $width,
    .header.h = $height,
    .data_size = sizeof(face_${name}_dial_img_${asset}_data_${a}),
    .header.cf = $color,
    .data = face_${name}_dial_img_${asset}_data_${a}};
"""

        text += obj

        val path = "assets_watchfaces/" + name
        val dir = File(path)

        if (!dir.exists()) {
            dir.mkdirs()
            println("Created output folder")
        }

        val fl = File(dir, "${name}_${asset}_${a}.c")
        fl.writeText(text)
    }
}

var lvItem =
        """
    {{CHILD}} = lv_img_create({{PARENT}});
    lv_img_set_src({{CHILD}}, ZSW_LV_IMG_USE({{RESOURCE}}));
    lv_obj_set_width({{CHILD}}, LV_SIZE_CONTENT);
    lv_obj_set_height({{CHILD}}, LV_SIZE_CONTENT);
    lv_obj_set_x({{CHILD}}, {{CHILD_X}});
    lv_obj_set_y({{CHILD}}, {{CHILD_Y}});
    lv_obj_add_flag({{CHILD}}, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag({{CHILD}}, LV_OBJ_FLAG_SCROLLABLE);
"""

var c_file =
        """
// File generated by bin2lvgl
// developed by fbiego.
// https://github.com/fbiego
// modified by Daniel Kampert.
// https://github.com/kampi
// Watchface: {{NAME}}

#include <lvgl.h>

#include <zephyr/logging/log.h>

#include "ui/zsw_ui.h"
#include "applications/watchface/watchface_app.h"

LOG_MODULE_REGISTER(watchface_{{NAME}}, LOG_LEVEL_WRN);

static lv_obj_t *face_{{NAME}};
static lv_obj_t *face_{{NAME}} = NULL;
static watchface_app_evt_listener ui_{{NAME}}_evt_cb;

static int last_date = -1;
static int last_day = -1;
static int last_month = -1;
static int last_year = -1;
static int last_weekday = -1;
static int last_hour = -1;
static int last_minute = -1;

{{OBJECTS}}
{{DECLARE}}
#if CONFIG_LV_COLOR_DEPTH_16 != 1
#error "CONFIG_LV_COLOR_DEPTH_16 should be 16 bit for watchfaces"
#endif
#if CONFIG_LV_COLOR_16_SWAP != 1
#error "CONFIG_LV_COLOR_16_SWAP should be 1 for watchfaces"
#endif

{{RSC_ARR}}
int32_t getPlaceValue(int32_t num, int32_t place) {
    int32_t divisor = 1;
    for (uint32_t i = 1; i < place; i++)
        divisor *= 10;
    return (num / divisor) % 10;
}

int32_t setPlaceValue(int32_t num, int32_t place, int32_t newValue) {
    int32_t divisor = 1;
    for (uint32_t i = 1; i < place; i++)
        divisor *= 10;
    return num - ((num / divisor) % 10 * divisor) + (newValue * divisor);
}

static void watchface_{{NAME}}_remove(void)
{
    if (!face_{{NAME}}) {
        return;
    }

    lv_obj_del(face_{{NAME}});
    face_{{NAME}} = NULL;
}

static void watchface_{{NAME}}_invalidate_cached(void)
{
    last_date = -1;
    last_day = -1;
    last_month = -1;
    last_year = -1;
    last_weekday = -1;
    last_hour = -1;
    last_minute = -1;
}

static void watchface_{{NAME}}_set_datetime(int day_of_week, int date, int day, int month, int year, int weekday, int hour,
                                   int minute, int second, uint32_t usec, bool am, bool mode)
{
    if (!face_{{NAME}}) {
        return;
    }

{{TIME}}
}

static void watchface_{{NAME}}_set_step(int32_t steps, int32_t distance, int32_t kcal)
{
    if (!face_{{NAME}}) {
        return;
    }

{{ACTIVITY}}
}

static void watchface_{{NAME}}_set_hrm(int32_t bpm, int32_t oxygen)
{
    if (!face_{{NAME}}) {
        return;
    }

{{HEALTH}}
}

static void watchface_{{NAME}}_set_weather(int8_t temperature, int icon)
{
    if (!face_{{NAME}}) {
        return;
    }

{{WEATHER}}
}

static void watchface_{{NAME}}_set_ble_connected(bool connected)
{
    if (!face_{{NAME}}) {
        return;
    }

{{CONNECTION}}
}

static void watchface_{{NAME}}_set_battery_percent(int32_t percent, int32_t battery)
{
    if (!face_{{NAME}}) {
        return;
    }

{{BATTERY}}
}

static void watchface_{{NAME}}_set_num_notifcations(int32_t number)
{
    if (!face_{{NAME}}) {
        return;
    }

{{NOTIFICATIONS}}
}

static void watchface_{{NAME}}_set_watch_env_sensors(int temperature, int humidity, int pressure, float iaq, float co2)
{
    if (!face_{{NAME}}) {
        return;
    }

{{ENVIRONMENT}}
}

void watchface_{{NAME}}_show(watchface_app_evt_listener evt_cb, zsw_settings_watchface_t *settings) {
    ui_{{NAME}}_evt_cb = evt_cb;

    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
    face_{{NAME}} = lv_obj_create(lv_scr_act());
    watchface_{{NAME}}_invalidate_cached();

    lv_obj_clear_flag(face_{{NAME}}, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(face_{{NAME}}, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(face_{{NAME}}, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_obj_set_style_border_width(face_{{NAME}}, 0, LV_PART_MAIN);
    lv_obj_set_size(face_{{NAME}}, 240, 240);
    lv_obj_clear_flag(face_{{NAME}}, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(face_{{NAME}}, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(face_{{NAME}}, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(face_{{NAME}}, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(face_{{NAME}}, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(face_{{NAME}}, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(face_{{NAME}}, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(face_{{NAME}}, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    {{ITEMS}}
}

static watchface_ui_api_t ui_api = {
    .show = watchface_{{NAME}}_show,
    .remove = watchface_{{NAME}}_remove,
    .set_battery_percent = watchface_{{NAME}}_set_battery_percent,
    .set_hrm = watchface_{{NAME}}_set_hrm,
    .set_step = watchface_{{NAME}}_set_step,
    .set_ble_connected = watchface_{{NAME}}_set_ble_connected,
    .set_num_notifcations = watchface_{{NAME}}_set_num_notifcations,
    .set_weather = watchface_{{NAME}}_set_weather,
    .set_datetime = watchface_{{NAME}}_set_datetime,
    .set_watch_env_sensors = watchface_{{NAME}}_set_watch_env_sensors,
    .ui_invalidate_cached = watchface_{{NAME}}_invalidate_cached,
};

static int watchface_{{NAME}}_init(void)
{
    watchface_app_register_ui(&ui_api);

    return 0;
}

SYS_INIT(watchface_{{NAME}}_init, APPLICATION, WATCHFACE_UI_INIT_PRIO);
"""

var asset_header =
        """
// File generated by bin2lvgl
// developed by fbiego.
// https://github.com/fbiego
// modified by Daniel Kampert.
// https://github.com/kampi
// Watchface: {{NAME}}

#include <lvgl.h>

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif
"""

val s = "\${app_sources}"
var cmake_file =
    """
# File generated by bin2lvgl
# developed by fbiego.
# https://github.com/fbiego
# modified by Daniel Kampert.
# https://github.com/kampi
# Watchface: {{NAME}}

FILE(GLOB app_sources *.c)
target_sources(app PRIVATE ${s})
"""