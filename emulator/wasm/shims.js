export function deleteNvsCookie() {
    window.localStorage.removeItem("nvs.json");
}

export function getNvsCookieSize() {
    const data = window.localStorage.getItem("nvs.json");

    if (data != null && typeof data != "undefined") {
        return data.length;
    } else {
        return 0;
    }
}

export function readNvsCookie(buf, size) {
    const data = window.localStorage.getItem("nvs.json");
    if (data != null && typeof data != "undefined") {
        const d = getArrUint8(buf);
        const trimmedData = data.substring(0, size);

        for (let i = 0; i < trimmedData.length; i++) {
            d[i] = trimmedData.charCodeAt(i) & 0xFF;
        }

        return trimmedData.length;
    }
}

export default function configure(imports, settings) {
    imports.env.deleteNvsCookie = deleteNvsCookie;
    imports.env.getNvsCookieSize = getNvsCookieSize;
    imports.env.readNvsCookie = readNvsCookie;
}
