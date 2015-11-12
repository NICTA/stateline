var defaultOptions = {
  'resolution': 1, // 1 second
  'windowSize': 10 // last minute
};

var Chart = function(endpoint, elemId, callback, options) {
  this.endpoint = endpoint;
  this.columns = [['x', 0]];
  this.callback = callback;
  this.chart = c3.generate({
    bindto: elemId,
    data: {
      x: 'x',
      columns: this.columns
    },
    point: {
      show: false
    },
    transition: {
      duration: 0
    },
    axis: {
      y: {
        tick: {
          format: d3.format('.2f')
        }
      },
    },
  });

  this.startTime = new Date().getTime() / 1000;

  this.options = defaultOptions;
  if (options) {
    for (var attr in options) { this.options[attr] = options[attr]; }
  }
};

Chart.prototype.update = function(data, config) {
  new_data = this.callback(data);
  for (key in new_data) {
    var idx = this.columns.map(function (col) { return col[0]; }).indexOf(key);
    if (idx == -1) {
      this.columns.push([key, new_data[key]]);
    } else {
      this.columns[idx].push(new_data[key]);
    }
  }

  this.columns[0].push(Math.floor(new Date().getTime() / 1000 - this.startTime));

  // Since we're only looking at a window, delete old data if we've reached window size
  if (this.columns[0].length - 1 > this.options['windowSize']) {
    for (var i = 0; i < this.columns.length; i++) {
      this.columns[i].splice(1, 1);
    }
  }

  var palette = ["#5DA5DA", "#FAA43A", "#60BD68", "#F17CB0",
    "#B2912F", "#B276B2", "#DECF3F", "#F15854"];
  var colors = [];
  for (var i = 0; i < config.stacks; i++) {
    colors.push.apply(colors, new gradStop({
      stops: config.chainsPerStack + 1,
      inColor: 'hex',
      colorArray: [palette[i], '#FFFFFF']
    }).slice(0, -1));
  }

  this.chart.load({
    columns: this.columns,
    colors: colors
  });
};
