const express = require('express')
const pinoHttp = require('pino-http')
const xml2js = require('xml2js')
const util = require('util')
const fs = require('fs')
const path = require('path')
const cp = require('child_process')
const Fraction = require('fraction.js')
const recursiveReadDir = require('recursive-readdir')
const { Observable } = require('@reactivex/rxjs')

const statAsync = util.promisify(fs.stat)

const logger = pinoHttp()
const app = express()
const parser = new xml2js.Parser()

let paths = {}

fs.readFile('../casparcg.config', function(err, data) {
    parser.parseString(data, function (err, result) {
        for (const path in result.configuration.paths[0]) {
            paths[path.split('-')[0]] = result.configuration.paths[0][path][0]
        }
        console.log('paths', paths)
    });
});

app.use(logger)

async function mediaInfo (fileDir, filePath) {
    const stat = await statAsync(filePath)
    if (stat.isDirectory()) {
        return
    }
    const { duration, timeBase, type } = await new Promise((resolve, reject) => {
        try {
            const args = [
                'M:/Server6/node/ffprobe.exe',
                '-hide_banner',
                '-i', `"${filePath}"`,
                '-show_format',
                '-show_streams',
                '-print_format', 'json'
            ]
            cp.exec(args.join(' '), (err, stdout, stderr) => {
                if (err) {
                    return reject(err)
                }
                const json = JSON.parse(stdout)
                if (!json.format || !json.streams || !json.streams[0]) {
                    return reject(new Error('not media'))
                }

                const timeBase = json.streams[0].time_base
                const duration = json.streams[0].duration_ts

                let type = 'AUDIO'
                if (json.format.duration < 1.0/25.0) {
                    type = 'STILL'
                } else if (json.streams[0].pix_fmt) {
                    type = 'MOVIE'
                }

                resolve({ type, duration, timeBase })
              });
        } catch (err) {
            reject(err)
        }
    })
    return [
        `"${path.relative(fileDir, filePath).replace(/\.[^/.]+$/, "")}"`,
        type,
        stat.size,
        stat.mtime.getTime(),
        duration,
        timeBase
    ].join(" ")
}

app.get('/cls', (req, res) => {
    const mediaPath = paths.media
    recursiveReadDir(mediaPath, async (err, paths) => {
        if (err) {
            // TODO
        }
        let str = "200 CLS OK\r\n"
        str += await Observable
            .from(paths)
            .mergeMap(async filePath => {
                try {
                    return `${ await mediaInfo(mediaPath, filePath)}\r\n`
                } catch (err) {
                    // TODO
                }
            }, null, 8)
            .filter(x => x)
            .reduce((acc, str) => acc + str)
            .toPromise()
        str += "\r\n"
        res.send(str)
    })
})

app.get('/tls', (req, res) => {
    const tlsPath = paths.template
    recursiveReadDir(tlsPath, (err, paths) => {
        if (err) {
            // TODO
        }
        let str = "200 TLS OK\r\n"
        for (const filePath of paths) {
            if (/\.ft$/.test(filePath)) {
                str += `${path.relative(tlsPath, filePath).replace(/\.[^/.]+$/, "")}\r\n`
            }
        }
        str += "\r\n"
        res.send(str)
    })
})

app.get('/fls', (req, res) => {
    // TODO
    let str = "200 FLS OK\r\n"
    str += "\r\n"
    res.send(str)
})

app.get('/cinf/:id', (req, res) => {
    const mediaPath = paths.media
    recursiveReadDir(mediaPath, async (err, paths) => {
        if (err) {
            // TODO
        }
        let fileId = req.params.id.toUpperCase()
        let filePath
        for (let filePath2 of paths) {
            const fileId2 = path.relative(mediaPath, filePath2).replace(/\.[^/.]+$/, "").toUpperCase()
            if (fileId2 === fileId) {
                filePath = filePath2
                break
            }
        }    
        if (!filePath) {
            // TODO
        }
        try {
            let str = "200 CINF OK\r\n"
            str += await mediaInfo(mediaPath, filePath)
            str += '\r\n\r\n'
            res.send(str)
        } catch (err) {
            // TODO
        }
    })
})

app.get('/thumbnail', (req, res) => {
    // TODO
})

app.get('/thumbnail/:id', (req, res) => {
    // TODO
})

app.get('/thumbnail/generate', (req, res) => {
    // TODO
})

app.get('/thumbnail/generate/:id', (req, res) => {
    // TODO
})

app.listen(8000, () => {

})