const path = require('path')

module.exports = {
  getId (fileDir, filePath) {
    return path
      .relative(fileDir, filePath)
      .replace(/\.[^/.]+$/, '')
      .replace(/\\+/g, '/')
      .toUpperCase()
  }
}
