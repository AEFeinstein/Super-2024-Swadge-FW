#!/usr/bin/python

from rme_view import view
from rme_model import model
from rme_controller import controller


def main():
    v: view = view()
    m: model = model(32, 16)
    c: controller = controller()

    c.setModel(m)
    v.setController(c)
    v.setModel(m)
    m.setView(v)

    v.redraw()

    v.mainloop()


if __name__ == '__main__':
    main()
