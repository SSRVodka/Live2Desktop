import os
import sys


def gen_qrc(resources_root_relative_dir: str, qrc_filename: str):
    cur_relative_path = resources_root_relative_dir
    cur_dir_list = os.listdir(resources_root_relative_dir)
    for item in cur_dir_list:
        cur_url = os.path.join(cur_relative_path, item)
        if os.path.isfile(cur_url):
            with open(qrc_filename, 'a+', encoding='utf-8') as file:
                file.write("\t<file>%s</file>\n" % cur_url.replace('\\', '/').lstrip("./"))
        else:
            gen_qrc(cur_url, qrc_filename)


if __name__ == '__main__':
    relative_dir = sys.argv[1]
    qrc_file = sys.argv[2]
    with open(qrc_file, 'a+', encoding='utf-8') as file:
        file.write("<RCC>\n<qresource>\n")
    gen_qrc(relative_dir, qrc_file)
    with open(qrc_file, 'a+', encoding='utf-8') as file:
        file.write("</qresource>\n</RCC>\n")
