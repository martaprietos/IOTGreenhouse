

var map = L.map('map').setView([53.276648, -9.011500], 18);

L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png', {
    maxZoom: 19,
    attribution: '&copy; <a href="http://www.openstreetmap.org/copyright">OpenStreetMap</a>'
}).addTo(map);


const changeText = document.querySelector("#change-text");

var marker1 = L.marker([53.276648, -9.011600]).addTo(map);
var marker2 = L.marker([53.276900, -9.01200]).addTo(map);
var marker3 = L.marker([53.276648, -9.011200]).addTo(map);
var marker4 = L.marker([53.276340, -9.01240]).addTo(map);

//marker1.bindPopup("<b>Greenhouse 1.").openPopup();
//marker2.bindPopup("<b>Greenhouse 2.").openPopup();
//marker3.bindPopup("<b>Greenhouse 3.").openPopup();
//marker4.bindPopup("<b>Greenhouse 4.").openPopup();

marker1.on('click', function(ev) {
    location.href='temp.html';
});
marker2.on('click', function(ev) {
    location.href='moist.html';
});
marker3.on('click', function(ev) {
    location.href='light.html';
});
marker4.on('click', function(ev) {
    location.href='water.html';
});