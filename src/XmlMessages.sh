function get_files
{
    echo x-k3b.xml
}
function po_for_file
{
    case "$1" in
       x-k3b.xml)
           echo x-k3b_xml_mimetypes.po
       ;;
    esac
}
function tags_for_file
{
    case "$1" in
      x-k3b.xml)
           echo comment
       ;;
    esac
}

