# Boost
> Expose 2011 SAAB 9-3 CAN interfaces over Bluetooth Low Energy.

This is a hobby project of mine to add some features to my car. While the constants within the project are pretty specific to my vehicle/deployment, it should be useful as a reference to anyone developing for their own vehicle.

## Features

- [x] Battery level monitoring
- [x] Steering wheel controls (send play/pause/skip commands to Spotify)
- [ ] Customizable-via-app button

## Hardware

- [Carloop with Redbear Duo](https://store.carloop.io/products/carloop-bluetooth)
- iOS Device


## Build Notes

Instructions on how to build the three moving parts are located in the README's within the subdirectories.
- [embedded device firmware](embedded)
- [client iOS app](iOS)
- [spotify token server](spotify)

## License

MIT