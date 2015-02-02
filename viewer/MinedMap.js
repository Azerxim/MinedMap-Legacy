var MinedMapLayer = L.GridLayer.extend({
	initialize: function (info) {
		this._info = info.info;
		this._regions = info.regions;

		if (!L.Browser.android) {
			this.on('tileunload', this._onTileRemove);
		}
	},

	createTile: function (coords, done) {
		var tile = document.createElement('img');

		tile.onload = L.bind(this._tileOnLoad, this, done, tile);
		tile.onerror = L.bind(this._tileOnError, this, done, tile);

		tile.alt = '';

		if (coords.x >= this._info.minX && coords.x <= this._info.maxX &&
		    coords.y >= this._info.minZ && coords.y <= this._info.maxZ &&
		    this._regions[coords.y-this._info.minZ][coords.x-this._info.minX])
			tile.src = 'data/r.'+coords.x+'.'+coords.y+'.png';

		return tile;
	},

	_tileOnLoad: function (done, tile) {
		done(null, tile);
	},

	_tileOnError: function (done, tile, e) {
		done(e, tile);
	},

	_getTileSize: function () {
		return 512;
	},

	_onTileRemove: function (e) {
		e.tile.onload = null;
		e.tile.src = L.Util.emptyImageUrl;
	},

	_abortLoading: function () {
		var i, tile;
		for (i in this._tiles) {
			tile = this._tiles[i].el;

			tile.onload = L.Util.falseFn;
			tile.onerror = L.Util.falseFn;

			if (!tile.complete) {
				tile.src = L.Util.emptyImageUrl;
				L.DomUtil.remove(tile);
			}
		}
	}
});


window.createMap = function () {
	var xhr = new XMLHttpRequest();
	xhr.onload = function () {
		var info = JSON.parse(this.responseText);

		var map = L.map('map', {
			center: [0, 0],
			zoom: 1,
			minZoom: 1,
			maxZoom: 1,
			crs: L.CRS.Simple,
			maxBounds: [
				[-256*(info.info.maxZ+1), 256*info.info.minX],
				[-256*info.info.minZ, 256*(info.info.maxX+1)],
			],
		});

		(new MinedMapLayer(info)).addTo(map);
	};

	xhr.open('GET', 'data/info.json', true);
	xhr.send();
}