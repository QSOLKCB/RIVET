#!/usr/bin/env python3
import argparse
import xml.etree.ElementTree as ET

ALLOWED_PRODUCT_NAMES = {
    "Windows Server 2012 R2 Datacenter Evaluation",
    "Microsoft Windows Server 2012 R2 Datacenter Evaluation",
}
ALLOWED_ARCHES = {"x86_64", "x86-64", "amd64"}

def collect(root, name):
    values = []
    for element in root.iter():
        if element.tag.split("}")[-1] == name and element.text is not None:
            values.append(element.text.strip())
    return values

def require_one(root, name):
    found = collect(root, name)
    if not found:
        raise ValueError(f"virt-inspector missing {name}")
    return found[0]

def validate(path):
    root = ET.parse(path).getroot()

    name = require_one(root, "name").lower()
    distro = require_one(root, "distro").lower()
    arch = require_one(root, "arch").lower()
    major = require_one(root, "major_version")
    minor = require_one(root, "minor_version")
    product_name = require_one(root, "product_name")
    product_variant = require_one(root, "product_variant")

    if name != "windows" or distro != "windows":
        raise ValueError(
            f"unexpected guest OS identity: name={name!r} distro={distro!r}"
        )
    if arch not in ALLOWED_ARCHES:
        raise ValueError(f"unexpected guest architecture: {arch!r}")
    if (major, minor) != ("6", "3"):
        raise ValueError(f"unexpected Windows version: {major}.{minor}")
    if product_variant.lower() != "server":
        raise ValueError(
            f"unexpected Windows product variant: {product_variant!r}"
        )
    if product_name not in ALLOWED_PRODUCT_NAMES:
        raise ValueError(
            f"unexpected Windows product name: {product_name!r}"
        )

    return {
        "name": name,
        "distro": distro,
        "arch": arch,
        "major_version": major,
        "minor_version": minor,
        "product_name": product_name,
        "product_variant": product_variant,
    }

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("inspector_xml")
    args = parser.parse_args()

    try:
        result = validate(args.inspector_xml)
    except (ET.ParseError, OSError, ValueError) as exc:
        raise SystemExit(str(exc))

    print(
        "validated guest:",
        result["product_name"],
        result["product_variant"],
        f'{result["major_version"]}.{result["minor_version"]}',
        result["arch"],
    )
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
