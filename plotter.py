import matplotlib.pyplot as plt
import numpy as np

class Plot(object):
  def __init__(self, axis_params):
    self.axis_params = axis_params
    self.fig, self.axes = plt.subplots(nrows = len(axis_params))
    if len(axis_params) == 1:
      self.axes = [self.axes]

    styles = ['r-', 'g-', 'b-', 'm-', 'k-', 'c-']
    
    def x_axis(i):
      return np.arange(0, axis_params[i][0], 1)

    plt.ion()

    def plot(i, ax):
      n, low, hi = axis_params[i]
      ax.axis([0, n, low, hi])
      return ax.plot(x_axis(i), [0] * n, 
          styles[i%len(styles)], animated=True)[0]
    self.lines = [plot(i, ax) for i, ax in enumerate(self.axes)]

    plt.draw()

    self.fig.show()
    #self.fig.canvas.draw()

    self.fig.canvas.mpl_connect('resize_event', self.on_resize)

    self.on_resize(None)

    
  def plot(self, subplot, ydata):
    self.lines[subplot].set_ydata(ydata)
    self.fig.canvas.restore_region(self.backgrounds[subplot])
    self.axes[subplot].draw_artist(self.lines[subplot])
    self.fig.canvas.blit(self.axes[subplot].bbox)

  def on_resize(self, event):
    for i, (n, low, hi) in enumerate(self.axis_params):
      self.lines[i].set_ydata([low - 1] * n)
    plt.draw()
    self.backgrounds = [self.fig.canvas.copy_from_bbox(ax.bbox) for ax in self.axes]

