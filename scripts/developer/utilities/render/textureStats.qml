//
//  stats.qml
//  examples/utilities/render
//
//  Created by Zach Pomerantz on 2/8/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4
import "../lib/plotperf"

Item {
    id: statsUI
    anchors.fill:parent

    Column {
        id: stats
        spacing: 8
        anchors.fill:parent
 
        property var config: Render.getConfig("Stats")
 
        function evalEvenHeight() {
            // Why do we have to do that manually ? cannot seem to find a qml / anchor / layout mode that does that ?
            return (height - spacing * (children.length - 1)) / children.length
        }

        PlotPerf {
            title: "Num Textures"
            height: parent.evalEvenHeight()
            object: stats.config
            plots: [
                {
                    prop: "textureCPUCount",
                    label: "CPU",
                    color: "#00B4EF"
                },
                {
                    prop: "textureGPUCount",
                    label: "GPU",
                    color: "#1AC567"
                },
                {
                    prop: "textureGPUTransferCount",
                    label: "Transfer",
                    color: "#9495FF"
                }
            ]
        }
        PlotPerf {
            title: "gpu::Texture Memory"
            height: parent.evalEvenHeight()
            object: stats.config
            valueScale: 1048576
            valueUnit: "Mb"
            valueNumDigits: "1"
            plots: [
                {
                    prop: "textureCPUMemoryUsage",
                    label: "CPU",
                    color: "#00B4EF"
                },
                {
                    prop: "textureGPUMemoryUsage",
                    label: "GPU",
                    color: "#1AC567"
                },
                {
                    prop: "textureGPUVirtualMemoryUsage",
                    label: "GPU Virtual",
                    color: "#9495FF"
                }
            ]
        }

        CheckBox {
            text: "Enable Texture Storage"
            checked: stats.config["textureStorageEnabled"]
            onCheckedChanged: { stats.config["textureStorageEnabled"] = checked }
        }
        CheckBox {
            text: "Enable Texture Transfer"
            checked: stats.config["textureTransferEnabled"]
            onCheckedChanged: { stats.config["textureTransferEnabled"] = checked }
        }
    }

}

