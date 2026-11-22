import base64
import io
import re
import zipfile
from pathlib import Path

src = Path(__file__).with_name("命令词播报词协议列表-voice-car-草案.xlsx")
buf = io.BytesIO()
with zipfile.ZipFile(src, "r") as zin, zipfile.ZipFile(buf, "w", zipfile.ZIP_DEFLATED) as zout:
    for item in zin.infolist():
        data = zin.read(item.filename)
        if item.filename == "xl/worksheets/sheet2.xml":
            continue
        if item.filename == "xl/_rels/workbook.xml.rels":
            text = data.decode("utf-8")
            text = re.sub(r"<Relationship[^>]*sheet2\.xml[^>]*/>", "", text)
            data = text.encode("utf-8")
        if item.filename == "xl/workbook.xml":
            text = data.decode("utf-8")
            text = re.sub(r"<sheet[^>]*r:id=\"rId2\"[^>]*/>", "", text)
            data = text.encode("utf-8")
        zout.writestr(item, data)
raw = buf.getvalue()
out = Path(__file__).with_name("_upload_b64.txt")
out.write_text(base64.b64encode(raw).decode("ascii"), encoding="ascii")
print(len(raw), len(out.read_text(encoding="ascii")))
